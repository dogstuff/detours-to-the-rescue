#version 330 core

in vec4 v_color;
in vec2 v_texcoord;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform float u_is_2d;
uniform float u_has_texture;
uniform float u_color_op;
uniform float u_color_arg1;
uniform float u_color_arg2;
uniform float u_alpha_op;
uniform float u_alpha_arg1;
uniform float u_alpha_arg2;

const float D3DTOP_DISABLE = 1.0;
const float D3DTOP_SELECTARG1 = 2.0;
const float D3DTOP_SELECTARG2 = 3.0;
const float D3DTOP_MODULATE = 4.0;

const float D3DTA_DIFFUSE = 0.0;
const float D3DTA_CURRENT = 1.0;
const float D3DTA_TEXTURE = 2.0;

vec4 select_stage_arg(float arg, vec4 diffuse_color, vec4 texture_color) {
    float selector = mod(arg, 16.0);

    if (abs(selector - D3DTA_TEXTURE) < 0.5) {
        return texture_color;
    }

    // On stage 0, D3DTA_CURRENT and D3DTA_DIFFUSE both resolve to diffuse.
    return diffuse_color;
}

vec4 combine_stage(float op, float arg1, float arg2, vec4 diffuse_color, vec4 texture_color) {
    if (abs(op - D3DTOP_DISABLE) < 0.5) {
        return diffuse_color;
    }

    vec4 a = select_stage_arg(arg1, diffuse_color, texture_color);
    vec4 b = select_stage_arg(arg2, diffuse_color, texture_color);

    if (abs(op - D3DTOP_SELECTARG1) < 0.5) {
        return a;
    }

    if (abs(op - D3DTOP_SELECTARG2) < 0.5) {
        return b;
    }

    // D3DTOP_MODULATE and unknown operations keep the legacy sidecar behavior.
    return a * b;
}

void main() {
    vec4 diffuse_color = v_color;
    vec4 texture_color = diffuse_color;

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

        texture_color = texture(u_texture, uv);
    }

    vec4 color_value = combine_stage(u_color_op, u_color_arg1, u_color_arg2, diffuse_color, texture_color);
    vec4 alpha_value = combine_stage(u_alpha_op, u_alpha_arg1, u_alpha_arg2, diffuse_color, texture_color);

    vec4 color = vec4(color_value.rgb, alpha_value.a);

    frag_color = color;
}
