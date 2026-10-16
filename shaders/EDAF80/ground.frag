/*
 * A simplified phong shader for the ground
 */
#version 410

uniform bool night_lighting;
uniform vec3 camera_position;
uniform vec3 camera_direction;
uniform vec3 flashlight_color;
uniform float flashlight_range;
uniform float flashlight_inner_angle;
uniform float flashlight_outer_angle;
uniform float flashlight_brightness;

out vec4 frag_color;

in VS_OUT {
    vec3 normal;
    vec2 texcoord;
    vec3 light;
    vec3 view;
    vec3 world_position;
} fs_in;

void main()
{

    vec3 ambient_color = vec3(18.0, 27.0, 24.0) / 255.0;
    vec3 diffuse_color = vec3(30.0, 39.0, 24.0) / 255.0;

    vec3 N = normalize(fs_in.normal);
    vec3 L = normalize(fs_in.light);
    vec3 V = normalize(fs_in.view);
    vec3 R = normalize(reflect(-L, N));

    vec3 diffuse_term = diffuse_color * max(dot(N, L), 0.0);
    vec3 color = ambient_color + diffuse_term;

    if (night_lighting) {
        color *= 0.06;

        // distance from the flashlight to this surface point
        vec3 to_fragment = fs_in.world_position - camera_position;
        float distance_to_light = length(to_fragment);
        float distance_fade = clamp(1.0 - distance_to_light / flashlight_range, 0.0, 1.0);

        // check whether the point is inside the light cone
        vec3 direction_to_fragment = normalize(to_fragment);
        float cos_angle = dot(direction_to_fragment, camera_direction);
        float inner_cos = cos(flashlight_inner_angle);
        float outer_cos = cos(flashlight_outer_angle);
        float cone_strength;

        if (cos_angle >= inner_cos) {
            cone_strength = 1.0;
        } else if (cos_angle <= outer_cos) {
            cone_strength = 0.0;
        } else {
            float edge_progress = (cos_angle - outer_cos) / (inner_cos - outer_cos);
            cone_strength = edge_progress * edge_progress * (3.0 - 2.0 * edge_progress);
        }

        if (cone_strength > 0.0) {
            vec3 flashlight_direction = -direction_to_fragment;
            float diffuse = max(dot(N, flashlight_direction), 0.0);
            color += diffuse_color * flashlight_color * flashlight_brightness * diffuse * distance_fade * cone_strength;
        }
    }

    frag_color.xyz = color;
    frag_color.w = 1.0;

}
