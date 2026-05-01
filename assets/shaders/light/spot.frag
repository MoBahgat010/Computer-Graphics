#version 330 core

in varyings {
    vec3 position;        // was FragPos
    vec3 normal;          // was Normal
    vec3 view_direction;  // was ViewDir
    vec4 color;           // was Color
    vec2 tex_coord;
} frag_in;

out vec4 frag_color;

struct SpotLight {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    vec3 position;
    vec3 direction;
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
    float inner_angle;
    float outer_angle;
};

uniform vec3      materialAmbient;
uniform vec3      materialDiffuse;
uniform vec3      materialSpecular;
uniform float     materialShininess;
uniform sampler2D diffuseMap;
uniform sampler2D specularMap;

uniform SpotLight light;

float calculate_lambert(vec3 normal, vec3 light_direction) {
    return max(0.0, dot(normal, -light_direction));
}

float calculate_phong(vec3 normal, vec3 light_direction, vec3 view, float shininess) {
    vec3 reflected = reflect(light_direction, normal);
    return pow(max(0.0, dot(view, reflected)), max(shininess, 1.0));
}

void main() {
    vec3 normal         = normalize(frag_in.normal);
    vec3 view_direction = normalize(frag_in.view_direction);

    vec3 light_direction = frag_in.position - light.position;
    float distance       = length(light_direction);
    light_direction      = normalize(light_direction);

    float attenuation = 1.0 / (light.attenuation_constant +
                                light.attenuation_linear    * distance +
                                light.attenuation_quadratic * distance * distance);

    float angle           = acos(dot(light.direction, light_direction));
    float angle_attenuation = smoothstep(light.outer_angle, light.inner_angle, angle);

    vec3 texColor  = texture(diffuseMap,  frag_in.tex_coord).rgb;
    vec3 specColor = texture(specularMap, frag_in.tex_coord).rgb;

    // vec3 albedo = (materialDiffuse  == vec3(0.0)) ? texColor  : materialDiffuse  * texColor;
    vec3 albedo = texColor;
    vec3 spec   = (materialSpecular == vec3(0.0)) ? specColor : materialSpecular * specColor;

    float diff_factor = calculate_lambert(normal, light_direction);
    float spec_factor = calculate_phong(normal, light_direction, view_direction, materialShininess);

    vec3 diffuse  = diff_factor * light.diffuse  * albedo;
    vec3 specular = spec_factor * light.specular * spec;

    vec3 lighting = (diffuse + specular) * attenuation * angle_attenuation;
    frag_color = frag_in.color * vec4(lighting, 1.0);
}