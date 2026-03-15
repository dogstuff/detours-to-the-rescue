#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in float a_rhw;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec2 a_texcoord;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texcoord;

layout(set = 1, binding = 0) uniform Uniforms {
    mat4 u_mvp;
    vec2 u_screen_size;
    float u_is_2d;
    float _pad;
};

void main() {
    v_color = a_color;
    v_texcoord = a_texcoord;

    if (u_is_2d > 0.5) {
        vec2 ndc = (a_position.xy / u_screen_size) * 2.0 - 1.0;
        ndc.y = -ndc.y;
        float w = 1.0 / a_rhw;
        gl_Position = vec4(ndc * w, a_position.z * w, w);
    } else {
        gl_Position = u_mvp * vec4(a_position, 1.0);
    }
}
