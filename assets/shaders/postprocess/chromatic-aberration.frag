#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;
out vec4 frag_color;

// How far (in the texture space) is the distance (on the x-axis) between
// the pixels from which the red/green (or green/blue) channels are sampled
#define STRENGTH 0.005

// Chromatic aberration mimics some old cameras where the lens disperses light
// differently based on its wavelength. In this shader, we will implement a
// cheap version of that effect

void main(){
    //TODO: Modify this shader to apply chromatic abberation
    // To apply this effect, we only read the green channel from the correct pixel (as defined by tex_coord)
    // To get the red channel, we move by amount STRENGTH to the left then sample another pixel from which we take the red channel
    // To get the blue channel, we move by amount STRENGTH to the right then sample another pixel from which we take the blue channel

    // Green: sampled from the correct pixel
    float g = texture(tex, tex_coord).g;

    // Red: sampled STRENGTH to the LEFT on the x-axis
    float r = texture(tex, tex_coord + vec2(-STRENGTH, 0.0)).r;

    // Blue: sampled STRENGTH to the RIGHT on the x-axis
    float b = texture(tex, tex_coord + vec2( STRENGTH, 0.0)).b;

    // Alpha: taken from the correct pixel
    float a = texture(tex, tex_coord).a;

    frag_color = vec4(r, g, b, a);
}
