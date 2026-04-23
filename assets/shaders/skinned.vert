#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;
layout(location = 4) in ivec4 boneIDs;
layout(location = 5) in vec4 boneWeights;

out Varyings {
    vec4 color;
    vec2 tex_coord;
} vs_out;

uniform mat4 transform;
uniform bool useAnimation;
uniform mat4 boneMatrices[200];

void main(){
    vec4 totalPosition = vec4(0.0);

    if(useAnimation) {
        bool hasBone = false;
        for(int i = 0; i < 4; i++) {
            if(boneIDs[i] >= 0 && boneIDs[i] < 200) {
                vec4 localPosition = boneMatrices[boneIDs[i]] * vec4(position, 1.0);
                totalPosition += localPosition * boneWeights[i];
                hasBone = true;
            }
        }
        if(!hasBone) {
            totalPosition = vec4(position, 1.0);
        }
    } else {
        totalPosition = vec4(position, 1.0);
    }

    gl_Position = transform * totalPosition;
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
}
