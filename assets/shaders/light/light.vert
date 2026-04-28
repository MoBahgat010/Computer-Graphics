#version 330 core

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 tex_coord;   // ADD
layout(location=3) in vec3 normal;

uniform mat4 transform;
uniform mat4 model;
uniform mat3 normalMatrix;
uniform vec3 cameraPos;

out varyings {
    vec3 position;
    vec3 normal;
    vec3 view_direction;
    vec4 color;
    vec2 tex_coord;              // ADD
} vert_out;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    vert_out.position = vec3(worldPos);
    
    vert_out.normal = normalize(normalMatrix * normal);
    
    vert_out.view_direction = normalize(cameraPos - vec3(worldPos));
    vert_out.tex_coord = tex_coord;
    vert_out.color = color;
    gl_Position = transform * vec4(position, 1.0);
}