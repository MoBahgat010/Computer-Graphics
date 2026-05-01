#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;

out vec4 frag_color;

uniform vec4 tint = vec4(1.0, 1.0, 1.0, 1.0);
uniform float damageStrength = 0.0;

// Vignette + blood drops along screen sides (damage effect)
void main(){
    // Convert texture coordinates (0 to 1) to NDC space (-1 to 1)
    vec2 ndc = tex_coord * 2.0 - 1.0;

    // Compute the squared distance from the center in NDC space
    float dist2 = dot(ndc, ndc);

    // Sample the scene color
    vec4 scene_color = texture(tex, tex_coord);

    // Blood overlay: only on the sides, with irregular drops
    float strength = clamp(damageStrength, 0.0, 1.0);
    if (strength <= 0.001) {
        frag_color = scene_color;
        return;
    }

    // Base vignette (only when taking damage)
    vec4 vignette_color = scene_color / (1.0 + dist2 * 0.8);

    // Side mask (stronger near left/right edges)
    float edge = smoothstep(0.55, 0.95, abs(ndc.x));

    // Cheap hash for irregular drops
    vec2 cell = vec2(floor(tex_coord.y * 45.0), floor(abs(ndc.x) * 18.0));
    float drop_noise = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
    float drops = smoothstep(0.6, 0.9, drop_noise);

    // Slightly stronger at the top, fading downwards
    float vertical = 1.0 - tex_coord.y;

    float blood_mask = edge * mix(0.35, 1.0, drops) * mix(0.4, 1.0, vertical);
    vec3 blood_color = vec3(0.7, 0.0, 0.0);

    vec3 final_color = mix(vignette_color.rgb, blood_color, clamp(blood_mask * strength, 0.0, 1.0));
    frag_color = vec4(final_color, vignette_color.a);
}
