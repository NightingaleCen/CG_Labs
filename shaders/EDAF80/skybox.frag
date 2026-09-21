#version 410

in VS_OUT {
	vec3 texcoord;
} fs_in;

uniform samplerCube cubemap;

out vec4 frag_color;

void main()
{
	frag_color = texture(cubemap, fs_in.texcoord);
}
