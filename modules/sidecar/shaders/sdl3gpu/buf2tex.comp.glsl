#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(set = 0, binding = 0) readonly buffer PixelData {
    uint pixels[];
};

layout(set = 1, binding = 0, rgba8) writeonly uniform image2D outTexture;

layout(set = 2, binding = 0) uniform Params {
    uint width;
    uint height;
};

void main() {
    uvec2 coord = gl_GlobalInvocationID.xy;
    if (coord.x >= width || coord.y >= height) return;

    uint pixel_index = coord.y * width + coord.x;
    uint packed_pixel = pixels[pixel_index];

    float b = float(packed_pixel & 0xFFu) / 255.0;
    float g = float((packed_pixel >> 8u) & 0xFFu) / 255.0;
    float r = float((packed_pixel >> 16u) & 0xFFu) / 255.0;
    float a = float((packed_pixel >> 24u) & 0xFFu) / 255.0;

    imageStore(outTexture, ivec2(coord), vec4(r, g, b, a));
}
