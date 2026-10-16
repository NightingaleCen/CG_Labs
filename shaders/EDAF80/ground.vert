#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;


out VS_OUT {
    vec3 normal;
	vec2 texcoord;
    vec3 light;
    vec3 view;
    vec3 world_position;
} vs_out;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 light_direction;
uniform vec3 camera_position;

void main()
{
    vec3 world_vertex = (vertex_model_to_world * vec4(vertex, 1.0)).xyz;
    vs_out.world_position = world_vertex;
    vs_out.normal = (vertex_model_to_world * vec4(normal, 0.0)).xyz;
    vs_out.view = camera_position - world_vertex;
    vs_out.light = light_direction;
	vs_out.texcoord = texcoord.xy;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
