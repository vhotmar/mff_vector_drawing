#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConsts {
    float scale;
    vec4 color;
} pc;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = pc.color;
}