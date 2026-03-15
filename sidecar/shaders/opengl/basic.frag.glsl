#version 330 core

in vec4 v_color;
in vec2 v_texcoord;

out vec4 frag_color;

uniform sampler2D u_texture;
uniform float u_is_2d;
uniform float u_has_texture;

void main() {
    vec4 color = v_color;

    if (u_has_texture > 0.5) {
        vec2 uv = v_texcoord;

        if (u_is_2d > 0.5) {
            vec2 tex_size = vec2(textureSize(u_texture, 0));

            if (u_is_2d > 1.5) {
                vec2 pixel = uv * tex_size;
                vec2 seam = floor(pixel + 0.5);
                vec2 dudv = fwidth(pixel);
                uv = (seam + clamp((pixel - seam) / dudv, -0.5, 0.5)) / tex_size;
            } else {
                uv = (floor(uv * tex_size) + 0.5) / tex_size;
            }
        }

        vec4 tex_color = texture(u_texture, uv);
        color *= tex_color;
    }

    frag_color = color;
}
