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
            vec2 uv_px = uv * tex_size;
            vec2 fl = floor(uv_px + 0.5);
            vec2 fr = fract(uv_px + 0.5);
            vec2 aa = fwidth(uv_px) * 2.0;
            fr = smoothstep(0.5 - aa, 0.5 + aa, fr);
            uv = (fl + fr - 0.5) / tex_size;
        }

        vec4 tex_color = texture(u_texture, uv);
        color *= tex_color;
    }

    frag_color = color;
}
