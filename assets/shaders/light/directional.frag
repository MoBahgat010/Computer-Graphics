#version 330 core

in varyings{
    vec3 position;
    vec3 normal;
    vec3 view_direction;
    vec4 color;
} frag_in;

out vec4 frag_color;

struct DirectionalLight{
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    
    // directional light specific properties
    vec3 direction;
}

// where to put in material?
uniform vec3 materialAmbient;    // Reflectivity
uniform vec3 materialDiffuse;    // Reflectivity
uniform vec3 materialSpecular;   // Reflectivity
uniform float materialShininess; 

uniform DirectionalLight directional_light;

float calculate_lambert(vec3 normal, vec3 light_direction){
        return max(0.0f, dot(normal, -light_direction));
}

    // This will be used to compute the phong specular.
float calculate_phong(vec3 normal, vec3 light_direction, vec3 view, float shininess){
    vec3 reflected = reflect(light_direction, normal);
    return pow(max(0.0f, dot(view, reflected)), shininess);
}

void main() {
    vec3 normal = normalize(frag_in.normal);
    vec3 view_direction = normalize(frag_in.view_direction);

    vec3 ambient = light.ambient * materialAmbient;
    float diffuse_factor = calculate_lambert(normal, directional_light.direction);
    vec3 diffuse = directional_light.diffuse * materialDiffuse * diffuse_factor;
    float specular_factor = calculate_phong(normal, directional_light.direction, view_direction, materialShininess);
    vec3 specular = directional_light.specular * materialSpecular * specular_factor;

    frag_color = vec4(ambient + diffuse + specular, 1.0f) * frag_in.color;
}