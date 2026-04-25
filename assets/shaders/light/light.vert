#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 tex_coord;
layout(location=3) in vec3 normal;

uniform mat4 transform;    // MVP
uniform mat4 model;        // model matrix (world space position)
uniform mat3 normalMatrix; // inverse-transpose of mat3(model)
uniform vec3 cameraPos;    // camera world position

out varyings {
    vec3 position;       // world space
    vec3 normal;         // world space, corrected
    vec3 view_direction; // from fragment toward camera
    vec4 color;
} vert_out;

void main(){
    vec4 worldPos = model * vec4(position, 1.0);
    vert_out.position = vec3(worldPos);
    vert_out.normal = normalize(normalMatrix * normal);
    vert_out.view_direction = normalize(cameraPos - vec3(worldPos));
    vert_out.color = color;
    gl_Position = transform * vec4(position, 1.0);
}