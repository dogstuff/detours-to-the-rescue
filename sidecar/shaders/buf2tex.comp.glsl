#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Set 0: readonly storage buffer (pixel data as packed uint32 BGRA)
layout(set = 0, binding = 0) readonly buffer PixelData {
    uint pixels[];
};

// Set 1: readwrite storage texture (output)
layout(set = 1, binding = 0, rgba8) writeonly uniform image2D outTexture;

// Set 2: uniform buffer (width/height via SDL_PushGPUComputeUniformData)
layout(set = 2, binding = 0) uniform Params {
    uint width;
    uint height;
};

void main() {
    uvec2 coord = gl_GlobalInvocationID.xy;
    if (coord.x >= width || coord.y >= height) return;

    uint idx = coord.y * width + coord.x;
    uint pixel = pixels[idx];

    // Unpack BGRA8888 to vec4 RGBA
    float b = float(pixel & 0xFFu) / 255.0;
    float g = float((pixel >> 8u) & 0xFFu) / 255.0;
    float r = float((pixel >> 16u) & 0xFFu) / 255.0;
    float a = float((pixel >> 24u) & 0xFFu) / 255.0;

    imageStore(outTexture, ivec2(coord), vec4(r, g, b, a));
}
