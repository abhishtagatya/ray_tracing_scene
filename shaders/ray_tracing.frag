#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
in VertexData
{
	vec2 tex_coord;
} in_data;

// The UBO with camera data.	
layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;	  // The projection matrix.
	mat4 projection_inv;  // The inverse of the projection matrix.
	mat4 view;			  // The view matrix
	mat4 view_inv;		  // The inverse of the view matrix.
	mat3 view_it;		  // The inverse of the transpose of the top-left part 3x3 of the view matrix
	vec3 eye_position;	  // The position of the eye in world space.
};

// The structure holding the information about a single Phong light.
struct PhongLight
{
	vec4 position;                   // The position of the light. Note that position.w should be one for point lights and spot lights, and zero for directional lights.
	vec3 ambient;                    // The ambient part of the color of the light.
	vec3 diffuse;                    // The diffuse part of the color of the light.
	vec3 specular;                   // The specular part of the color of the light. 
	vec3 spot_direction;             // The direction of the spot light, irrelevant for point lights and directional lights.
	float spot_exponent;             // The spot exponent of the spot light, irrelevant for point lights and directional lights.
	float spot_cos_cutoff;           // The cosine of the spot light's cutoff angle, -1 point lights, irrelevant for directional lights.
	float atten_constant;            // The constant attenuation of spot lights and point lights, irrelevant for directional lights. For no attenuation, set this to 1.
	float atten_linear;              // The linear attenuation of spot lights and point lights, irrelevant for directional lights.  For no attenuation, set this to 0.
	float atten_quadratic;           // The quadratic attenuation of spot lights and point lights, irrelevant for directional lights. For no attenuation, set this to 0.
};

// The UBO with light data.
layout (std140, binding = 2) uniform PhongLightsBuffer
{
	vec3 global_ambient_color;		// The global ambient color.
	int lights_count;				// The number of lights in the buffer.
	PhongLight lights[3];			// The array with actual lights.
};

// The material data.
struct PBRMaterialData
{
   vec3 diffuse;     // The diffuse color of the material.
   float roughness;  // The roughness of the material.
   vec3 f0;          // The Fresnel reflection at 0.
};

// The UBO with Model and Material Data
layout (std140, binding = 3) uniform ModelBuffer
{
	vec4 spheres[13]; // The spheres in the scene.
	PBRMaterialData materials[13]; // The materials of the spheres.
};

// The resolution of the screen.
uniform vec2 resolution;

// The number of spheres to render.
uniform int spheres_count;

// The number of iterations.
uniform int iterations;

// The number of shadow samples.
uniform int shadow_samples;

// Time variable
uniform float time;

// Use ambient occlusion
uniform bool use_ambient_occlusion;

// Ambient occlusion samples
uniform int ambient_occlusion_samples;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
// The final output color.
layout (location = 0) out vec4 final_color;

// ----------------------------------------------------------------------------
// Ray Tracing Structures
// ----------------------------------------------------------------------------
// The definition of a ray.
struct Ray {
    vec3 origin;     // The ray origin.
    vec3 direction;  // The ray direction.
};
// The definition of an intersection.
struct Hit {
    float t;				  // The distance between the ray origin and the intersection points along the ray. 
	vec3 intersection;        // The intersection point.
    vec3 normal;              // The surface normal at the interesection point.
	PBRMaterialData material; // The material of the object at the intersection point.
	bool isLight;             // The flag determining whether the object is a light source.
};
const Hit miss = Hit(1e20, vec3(0.0), vec3(0.0), PBRMaterialData(vec3(0),0,vec3(0)), false);

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// Local Methods
// ----------------------------------------------------------------------------

// The FresnelSchlick approximation of the reflection.
vec3 FresnelSchlick(in vec3 f0, in vec3 V, in vec3 H)
{
	return f0 + (1.0 - f0) * pow(1.0 - clamp(dot(V, H), 0.0, 1.0), 5.0);
}

// Computes an intersection between a ray and a sphere defined by its center and radius.
// ray - the ray definition (contains ray.origin and ray.direction)
// center - the center of the sphere
// radius - the radius of the sphere
// i - the index of the sphere in the array, can be used to obtain the material from materials buffer
Hit RaySphereIntersection(Ray ray, vec3 center, float radius, int i, bool isLight) {
	
	vec3 oc = ray.origin - center;
	float b = dot(ray.direction, oc);
	float c = dot(oc, oc) - (radius*radius);

	float det = b*b - c;
	if (det < 0.0) return miss;

	float t = -b - sqrt(det);
	if (t < 0.0) t = -b + sqrt(det);
	if (t < 0.0) return miss;

	vec3 intersection = ray.origin + t * ray.direction;
	vec3 normal = normalize(intersection - center);
    return Hit(t, intersection, normal, materials[i], isLight);
}

// Computes an intersection between a ray and a plane defined by its normal and one point inside the plane.
// ray - the ray definition (contains ray.origin and ray.direction)
// normal - the plane normal
// point - a point laying in the plane
Hit RayPlaneIntersection(Ray ray, vec3 normal, vec3 point) {
	float nd = dot(normal, ray.direction);
	vec3 sp = point - ray.origin;
	float t = dot(sp, normal) / nd;
    if (t < 0.0) return miss;

	vec3 intersection = ray.origin + t * ray.direction;

	if(intersection.x > 40 || intersection.x < -40 || intersection.z > 40 || intersection.z < -40) return miss;

    return Hit(t, intersection, normal, materials[0], false);
}

// Evaluates the intersections of the ray with the scene objects and returns the closes hit.
Hit Evaluate(Ray ray){
	// Sets the closes hit either to miss or to an intersection with the plane representing the ground.
	Hit closest_hit = RayPlaneIntersection(ray, vec3(0, 1, 0), vec3(0));
	
	for(int i = 0; i < spheres_count; i++){
		vec3 center = spheres[i].xyz;

		Hit intersection = RaySphereIntersection(ray, center, spheres[i].w, i, i >= 10);
		if(intersection.t < closest_hit.t){
			closest_hit = intersection;
		}
	}

    return closest_hit;
}

Hit EvaluateExcludeLight(Ray ray){
	// Sets the closes hit either to miss or to an intersection with the plane representing the ground.
	Hit closest_hit = RayPlaneIntersection(ray, vec3(0, 1, 0), vec3(0));
	
	for(int i = 0; i < spheres_count - 3; i++){
		vec3 center = spheres[i].xyz;

		Hit intersection = RaySphereIntersection(ray, center, spheres[i].w, i, i >= 10);
		if(intersection.t < closest_hit.t){
			closest_hit = intersection;
		}
	}

    return closest_hit;
}

float random(vec2 p) {
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); // From ShaderToy (https://www.shadertoy.com/view/4djSRW)
}

float SphereOcclusion(Hit hit) {
    float occlusion = 0.0;
	float epsilon = 1e-2;

    // Randomly distribute rays in a hemisphere above the hit point
    for (int i = 0; i < ambient_occlusion_samples; ++i) {
        // Generate a random point in a hemisphere using spherical coordinates
        float phi = random(vec2(hit.intersection.x, hit.intersection.y + float(i))) * 2.0 * PI;
        float theta = random(vec2(hit.intersection.y, hit.intersection.z + float(i))) * 0.5 * PI;

        // Convert spherical coordinates to Cartesian coordinates
        vec3 sample_direction = vec3(
            sin(theta) * cos(phi),
            cos(theta),
            sin(theta) * sin(phi)
        );

        // Direction of the ray to test occlusion
        Ray sample_ray = Ray(hit.intersection + epsilon * hit.normal, sample_direction);  // Slightly offset from surface

        // Evaluate if the ray hits any objects in the scene (excluding light sources)
        Hit occlusion_hit = EvaluateExcludeLight(sample_ray);

        // If the ray intersects another object, increase occlusion
        if (occlusion_hit != miss) {
            occlusion += 1.0;
        }
    }

    // Return occlusion factor (0 to 1), more occlusion means darker
    return 1.0 - (occlusion / ambient_occlusion_samples);
}

vec3 Trace(Ray ray) {

	vec3 color = vec3(0.0);
	vec3 attenuation = vec3(1.0);
	float epsilon = 1e-2;

	for (int i = 0; i < iterations; ++i) {
		Hit hit = Evaluate(ray);
		if (hit == miss) return color;

		vec3 V = -ray.direction;
		vec3 fresnel = FresnelSchlick(hit.material.f0, V, hit.normal);

		if (hit.isLight) {
			color +=  hit.material.diffuse * (1 - fresnel) * attenuation;
			break;
		}

		float ao = 1.0;
		if (use_ambient_occlusion) {
			ao = SphereOcclusion(hit);
		}

		for (int j = 0; j < lights_count; j++) {
			vec3 light_position = lights[j].position.xyz;

			vec3 L_not_normalize = light_position - hit.intersection;
			vec3 L = normalize(L_not_normalize);
			vec3 T = normalize(cross(L, vec3(0, -1, 0))); // Tangent
			vec3 B = normalize(cross(L, T)); // Bitangent
			
			float distance_from_light = length(L_not_normalize);

			float radius = lights[j].position.w / distance_from_light;
			float atten_factor = 1.0 / (1 + 0.5 * distance_from_light);
            
			vec3 color_t = vec3(0.0);

			for (int k = 0; k < shadow_samples; k++) {
				float v = float(k+1)*.152;
				float random_value = random(vec2(gl_FragCoord.x, gl_FragCoord.y + j) * v + time * 1500. + 50.0);
				float random_angle = 2 * PI * random_value;
				float random_radius = radius * sqrt(random_value);

				vec2 point_on_disk = vec2(cos(random_angle), sin(random_angle)) * random_radius;

				vec3 shadow_ray_direction = L + point_on_disk.x * T + point_on_disk.y * B;

				Ray shadow_ray = Ray(hit.intersection + epsilon * L, shadow_ray_direction);
				Hit shadow_hit = EvaluateExcludeLight(shadow_ray);
				if (shadow_hit == miss || shadow_hit.t >= distance_from_light) {
					color_t += max(dot(hit.normal, L), 0.0) * lights[j].diffuse * hit.material.diffuse * (1.0 - fresnel) * atten_factor * attenuation * ao; 
				}
			}

			color += color_t / shadow_samples;
		}

		attenuation *= hit.material.diffuse * fresnel;

		vec3 reflection = reflect(ray.direction, hit.normal);
		ray = Ray(hit.intersection + epsilon * reflection, reflection);
	}

	return color;
}

float TraceDepth(Ray ray) {
	Hit hit = Evaluate(ray);
	return hit.t;
}

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	float aspect_ratio = resolution.x/resolution.x;

	// We use the texture coordinates and the aspect ratio to map the coordinates to the screen space.
	vec2 uv = (2.0 * in_data.tex_coord - 1.0) * vec2(aspect_ratio, 1.0);

	// Computes the ray origin and ray direction using the view matrix.
	vec3 P = vec3(view_inv * projection_inv * vec4(uv, -1.0, 1.0));
	vec3 direction = normalize(P - eye_position);
	Ray ray = Ray(eye_position, direction);

	// We pass the ray to the trace function.
	vec3 color = Trace(ray);

	// Calculate depth based on the real_distance using the equation
    float near = 1.0;
    float far = 1000.0;
	float real_distance = TraceDepth(ray);
    float depth = (1.0 / real_distance - 1.0 / near) / (1.0 / far - 1.0 / near);

    // Set the fragment depth
    gl_FragDepth = depth;


	final_color = vec4(color, 1.0);
}