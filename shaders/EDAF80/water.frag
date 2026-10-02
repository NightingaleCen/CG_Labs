#version 410

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 view;

	vec2 normal_texcoord1;
	vec2 normal_texcoord2;
	vec2 normal_texcoord3;
} fs_in;

out vec4 frag_color;

uniform samplerCube cubemap;
uniform sampler2D normal_texture;

void main()
{
	const vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
	const vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);

	vec3 normal1 = texture(normal_texture, fs_in.normal_texcoord1).xyz * 2.0 - 1.0;
	vec3 normal2 = texture(normal_texture, fs_in.normal_texcoord2).xyz * 2.0 - 1.0;
	vec3 normal3 = texture(normal_texture, fs_in.normal_texcoord3).xyz * 2.0 - 1.0;

	vec3 bump_normal = normalize(normal1 + normal2 + normal3);
	mat3 basis = mat3(normalize(fs_in.tangent), normalize(fs_in.binormal), normalize(fs_in.normal));

	vec3 V, N, R;
	V = normalize(fs_in.view);
	N = normalize(vec4(basis * bump_normal, 0.0).xyz);
	R = reflect(-V, N);

	float facing = 1.0 - max(0.0, dot(V, N));
	vec4 color_water = mix(color_deep, color_shallow, facing);

	vec4 color_reflection = texture(cubemap, R);

	const float R0 = 0.02037;
	float fresnel = R0 + (1.0 - R0) * pow(1.0 - max(0.0, dot(V, N)), 5.0);

	const float eta = 1.0 / 1.333;
	vec4 refraction = texture(cubemap, refract(-V, N, eta));

	//frag_color = texture(cubemap, refract(-V, normalize(fs_in.normal), eta));
	frag_color = color_water + color_reflection * fresnel + refraction * (1.0 - fresnel);
}
