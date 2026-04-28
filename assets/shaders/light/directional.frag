#version 330 core

in varyings {
    vec3 position;
    vec3 normal;
    vec3 view_direction;
    vec4 color;
    vec2 tex_coord;
} frag_in;

out vec4 frag_color;

struct DirectionalLight {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    vec3 direction;
};

uniform vec3  materialDiffuse;
uniform vec3  materialSpecular;
uniform float materialShininess;
uniform sampler2D diffuseMap;
uniform DirectionalLight directional_light;


void main() {
    // FIX 1: Normalize inputs properly
    vec3 n = normalize(frag_in.normal);
    vec3 v = normalize(frag_in.view_direction);
    vec3 l = normalize(directional_light.direction);

    // FIX 2: Texture fallback
    vec4 texSample = texture(diffuseMap, frag_in.tex_coord);
    vec3 albedo = (texSample.a < 0.01) ? materialDiffuse : texSample.rgb;

    // FIX 3: Correct Lambert (use -l because light direction is 'to' the surface)
    float diff_factor = max(0.0, dot(n, -l));
    vec3 diffuse = directional_light.diffuse * diff_factor * albedo;

    // FIX 4: Correct Phong
    vec3 reflected = reflect(-l, n);
    float spec_factor = pow(max(0.0, dot(v, reflected)), max(materialShininess, 1.0));
    vec3 specular = directional_light.specular * materialSpecular * spec_factor;

    

    frag_color = vec4(diffuse + specular, 1.0) * frag_in.color;
}