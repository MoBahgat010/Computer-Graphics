#version 330 core

in varyings {
    vec3 position;
    vec3 normal;
    vec3 view_direction;
    vec4 color;
    vec2 tex_coord;
} frag_in;

out vec4 frag_color;

uniform vec3 materialAmbient;
uniform sampler2D diffuseMap;

void main() {
    vec3 texColor = texture(diffuseMap, frag_in.tex_coord).rgb;

    vec3 ambient = texColor * materialAmbient;

    frag_color = vec4(ambient, 1.0) * frag_in.color;
}