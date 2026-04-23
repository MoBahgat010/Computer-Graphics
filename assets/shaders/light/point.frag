#version 330 core


in varyings{
    vec3 position;
    vec3 normal;
    vec3 view_direction;
    vec4 color;
} frag_in;

out vec4 frag_color;


struct PointLight {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;

    // spot light specific properties
    vec3 position;

    // The attenuation is used to control how the light dims out as we go further from it.
    float attenuation_constant;
    float attenuation_linear;
    float attenuation_quadratic;
};

uniform vec3 materialAmbient;    // Reflectivity
uniform vec3 materialDiffuse;    // Reflectivity
uniform vec3 materialSpecular;   // Reflectivity
uniform float materialShininess; 

uniform PointLight light;

float calculate_lambert(vec3 normal, vec3 light_direction){
        return max(0.0f, dot(normal, -light_direction));
}

float calculate_phong(vec3 normal, vec3 light_direction, vec3 view, float shininess){
    vec3 reflected = reflect(light_direction, normal);
    return pow(max(0.0f, dot(view, reflected)), shininess);
}

void main()
{
    vec3 normal = normalize(frag_in.normal);
    vec3 view_direction = normalize(frag_in.view_direction);

    vec3 light_direction = frag_in.position - light.position;
    float distance = length(light_direction);
    light_direction = normalize(light_direction);

    // Get the attenuation factor based on the light distance from the pixel
    float attenuation = 1.0f / (light.attenuation_constant +
                                light.attenuation_linear * distance +
                                light.attenuation_quadratic * distance * distance);

    vec3 ambient = light.ambient * materialAmbient;
    float diffuse_factor = calculate_lambert(normal, light_direction);
    vec3 diffuse = diffuse_factor * light.diffuse * materialDiffuse;
    float specular_factor = calculate_phong(normal, light_direction, view_direction, materialShininess);
    vec3 specular = specular_factor * light.specular * materialSpecular;

    vec3 lighting = (diffuse + specular) * attenuation + ambient;
    frag_color = frag_in.color * vec4(lighting, 1.0);
}
