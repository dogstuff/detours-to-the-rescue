#version 450

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;

layout(location = 0) out vec4 frag_color;

layout(set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0) uniform Uniforms {
    mat4 u_mvp;
    vec2 u_screen_size;
    float u_is_2d;
    float u_has_texture;
};

void main() {
    vec4 color = v_color;

    if (u_has_texture > 0.5) {
        vec2 uv = v_texcoord;

        if (u_is_2d > 0.5) {
            vec2 tex_size = vec2(textureSize(u_texture, 0));

            if (u_is_2d > 1.5) {
                vec2 pixel = uv * tex_size;
                vec2 seam = floor(pixel + 0.5);
                vec2 pixel_span = fwidth(pixel);
                uv = (seam + clamp((pixel - seam) / pixel_span, -0.5, 0.5)) / tex_size;
            } else {
                uv = (floor(uv * tex_size) + 0.5) / tex_size;
            }
        }

        color *= texture(u_texture, uv);
    }

    frag_color = color;
}
