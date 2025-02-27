#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
in VertexData
{
	vec3 color;	       // The particle color.
	vec2 tex_coord;    // The texture coordinates for the particle.
	float lifetime;    // The lifetime of the particle.
	float delay;	   // The delay of the particle.
} in_data;

// The particle texture.
layout (binding = 0) uniform sampler2D particle_texture;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
// The final fragment color.
layout (location = 0) out vec4 final_color;

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	// TASK 4: Use the intensity from the texture to modify color and opacity of the frament.
	//  Hints: Note that the texture contains only shades of gray.
	//         The color of the particle is defined in in_data.color
	float intensity = texture(particle_texture, in_data.tex_coord).r;
	float opacity = intensity * 0.5 + 0.5; // Modify opacity to be between 0.0 and 1.0
	final_color = vec4(in_data.color * intensity * (in_data.delay / in_data.lifetime), intensity);
}