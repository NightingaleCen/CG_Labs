#version 410

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec2 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float time;
uniform vec3 camera_position;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 view;

	vec2 normal_texcoord1;
	vec2 normal_texcoord2;
	vec2 normal_texcoord3;
} vs_out;

void get_alpha(out float alpha, in vec2 position, 
		  	   in vec2 direction, in float frequency, in float phase)
{
	float sin_term;
	sin_term = (direction.x * position.x + direction.y * position.y) * frequency + time * phase;
	alpha = 0.5 * sin(sin_term) + 0.5;
}

void wave(out float height, in float t, in vec2 position, 
		  in float amplitude, in vec2 direction, in float frequency, in float phase, in float sharpness)
{
	float alpha;
	get_alpha(alpha, position, direction, frequency, phase);
	height = amplitude * pow(alpha, sharpness);
}

void get_partial(out vec2 partial, in vec2 position, 
		  		 in float amplitude, in vec2 direction, in float frequency, in float phase, in float sharpness)
{
	float partial_x, partial_z;

	float alpha;
	get_alpha(alpha, position, direction, frequency, phase);

	float cos_term = (direction.x * position.x + direction.y * position.y) * frequency + time * phase;
	partial_x = 0.5 * sharpness * frequency * amplitude * pow(alpha, sharpness - 1.0) * cos(cos_term) * direction.x;
	partial_z = 0.5 * sharpness * frequency * amplitude * pow(alpha, sharpness - 1.0) * cos(cos_term) * direction.y;

	partial = vec2(partial_x, partial_z);
}


void main()
{
	// Vertex
	float wave1, wave2, wave_combined;
	wave(wave1, time, vertex.xz, 1.0, vec2(-1.0, 0.0), 0.2, 0.5, 2.0);
	wave(wave2, time, vertex.xz, 0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0);

	wave_combined = wave1 + wave2;

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex.x, wave_combined, vertex.z, 1.0));
	vs_out.view = camera_position - vs_out.vertex;

	// TBN
	vec2 partial1, partial2, partial_combined;
	get_partial(partial1, vertex.xz, 1.0, vec2(-1.0, 0.0), 0.2, 0.5, 2.0);
	get_partial(partial2, vertex.xz, 0.5, vec2(-0.7, 0.7), 0.4, 1.3, 2.0);
	partial_combined = partial1 + partial2;

	vs_out.normal = vec3(normal_model_to_world * vec4(-partial_combined.x, 1.0, -partial_combined.y, 0.0));
	vs_out.tangent = vec3(normal_model_to_world * vec4(1.0, partial_combined.x, 0.0, 0.0));
	vs_out.binormal = vec3(normal_model_to_world * vec4(0.0, partial_combined.y, 1.0, 0.0));

	// Normal texture coordinates
	vec2 texScale = vec2(8, 4); 
	float normalTime = mod(time, 100.0); 
	vec2 normalSpeed = vec2(-0.05, 0.0); 

	vs_out.normal_texcoord1 =  texcoord.xy * texScale + normalTime * normalSpeed; 
	vs_out.normal_texcoord2 =  texcoord.xy * texScale * 2 + normalTime * normalSpeed * 4; 
	vs_out.normal_texcoord3 =  texcoord.xy * texScale * 4 + normalTime * normalSpeed * 8;


	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex.x, wave_combined, vertex.z, 1.0);
}



