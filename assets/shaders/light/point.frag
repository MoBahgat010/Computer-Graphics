#version 330 core

in varyings {
    vec3 position;
    vec3 normal;
    vec3 view_direction;
    vec4 color;
    vec2 tex_coord;
} frag_in;

out vec4 frag_color;

struct PointLight {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    vec3 position;
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
};

uniform vec3      materialAmbient;
uniform vec3      materialDiffuse;
uniform vec3      materialSpecular;
uniform float     materialShininess;
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

uniform PointLight light;

float calculate_lambert(vec3 normal, vec3 light_direction) {
    return max(0.0, dot(normal, -light_direction));
}

float calculate_phong(vec3 normal, vec3 light_direction, vec3 view, float shininess) {
    vec3 reflected = reflect(light_direction, normal);
    return pow(max(0.0, dot(view, reflected)), max(shininess, 1.0));
}

void main() {
    vec3 n = normalize(frag_in.normal);
    vec3 v = normalize(frag_in.view_direction);

    // FIX 1: Vector from Fragment TO Light
    vec3 light_dir_full = light.position - frag_in.position; 
    float dist = length(light_dir_full);
    vec3 l = normalize(light_dir_full);

    // FIX 2: Attenuation
    float attenuation = 1.0 / (light.attenuation_constant +
                                light.attenuation_linear * dist +
                                light.attenuation_quadratic * (dist * dist));
    
    vec3 albedo = texture(diffuseMap, frag_in.tex_coord).rgb;
    if(length(albedo) < 0.01) albedo = materialDiffuse;

    // FIX 3: dot(n, l) because l points TO light
    float diff_factor = max(0.0, dot(n, l));
    vec3 diffuse = diff_factor * light.diffuse * albedo;

    // FIX 4: Specular
    vec3 reflected = reflect(-l, n);
    float spec_factor = pow(max(0.0, dot(v, reflected)), max(materialShininess, 1.0));
    vec3 specular = spec_factor * light.specular * materialSpecular;

    frag_color = vec4((diffuse + specular) * attenuation, 1.0) * frag_in.color;
}