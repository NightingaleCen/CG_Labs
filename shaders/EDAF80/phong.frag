#version 410

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform int use_texture;
uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;


uniform int use_normal_mapping;
uniform sampler2D normal_texture;

in VS_OUT {
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 view;
    vec3 light;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
    vec3 diffuse_term;
    vec3 specular_term;

	vec3 N;
    if (use_normal_mapping != 0 ) {
        vec3 bump_normal = texture(normal_texture, fs_in.texcoord).xyz * 2.0 - 1.0;
        mat3 basis = mat3(normalize(fs_in.tangent), normalize(fs_in.binormal), normalize(fs_in.normal));
        N = normalize(vec4(basis * bump_normal, 0.0).xyz);
    } else {
        N = normalize(fs_in.normal);
    }

    vec3 L = normalize(fs_in.light);
    vec3 V = normalize(fs_in.view);
    vec3 R = normalize(reflect(-L, N));

    if (use_texture != 0) {
        vec3 diffuse_tex = texture(diffuse_texture, fs_in.texcoord).rgb;
        vec3 specular_tex = texture(specular_texture, fs_in.texcoord).rgb;
        diffuse_term = diffuse_tex * max(dot(N, L), 0.0);
        specular_term = specular_tex * pow(max(dot(R, V), 0.0), shininess);
    } else {
        diffuse_term = diffuse * max(dot(N, L), 0.0);
        specular_term = specular * pow(max(dot(R, V), 0.0), shininess);
    }
    
    frag_color.xyz = ambient + diffuse_term + specular_term;
    frag_color.w = 1.0;
}
