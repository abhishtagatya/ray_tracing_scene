#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------

// The UBO with camera data.	
layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;		// The projection matrix.
	mat4 projection_inv;	// The inverse of the projection matrix.
	mat4 view;				// The view matrix
	mat4 view_inv;			// The inverse of the view matrix.
	mat3 view_it;			// The inverse of the transpose of the top-left part 3x3 of the view matrix
	vec3 eye_position;		// The position of the eye in world space.
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

uniform float t_delta;	// The current time.
uniform vec3 gravity = vec3(0.0, -9.81, 0.0);  // Gravity in the Y direction
uniform float light_radius; // The radius of the light source

struct Particle {
	vec4 position;	// The position of the particle.
	vec3 velocity;	// The velocity of the particle.
	int light_id;	// The id of the light that should be used for this particle.
	vec3 color;		// The color of the particle.
	float delay;	// The delay before the particle should start moving.
	float lifetime; // The lifetime of the particle.
};

layout (std430, binding = 3) buffer ParticleBuffer
{
	Particle particles[]; // The array with particles.
};

// Function to generate a random number based on input (simple hash function)
float random(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec3 random_direction(float min, float max)
{
    return vec3(
        random(gl_VertexID + 1) * (max - min) + min, // X component
        random(gl_VertexID + 2) * (max - min) + min, // Y component
        random(gl_VertexID + 3) * (max - min) + min  // Z component
    );
}

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 color;	       // The particle color.
	vec4 position_vs;  // The particle position in view space.
	float lifetime;    // The lifetime of the particle.
	float delay;	   // The delay of the particle.
} out_data;


// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	Particle particle = particles[gl_VertexID];

	vec4 light_position = lights[particle.light_id].position;
	vec3 color = lights[particle.light_id].diffuse;

    if(particle.delay < 0 || particle.position.w == 0.0f || particle.position.y < 0){
	    particle.light_id = int(mod(float(gl_VertexID), float(lights_count))); // evenly distribute based on lights_count

		vec3 rand_dir = random_direction(-1,1);
		rand_dir = normalize(rand_dir);
		particle.position = vec4(light_position.xyz + rand_dir * light_radius, 1);

		particle.velocity = rand_dir * 1.5;
		particle.lifetime = random(gl_VertexID);
		particle.delay = particle.lifetime;
		particle.color = color;
	}

	particle.delay -= t_delta;

	// Update the particle's position based on its velocity
	particle.position += vec4(particle.velocity, 0) * t_delta + 0.5f * vec4(gravity, 0) * t_delta * t_delta;
	particle.velocity += gravity * t_delta;

    // Set the particle's position back into the buffer
    particles[gl_VertexID] = particle;

    // Output gl_Position for the current particle
	out_data.color = particle.color;
    out_data.position_vs = view * particle.position;
	out_data.lifetime = particle.lifetime;
	out_data.delay = particle.delay;
}