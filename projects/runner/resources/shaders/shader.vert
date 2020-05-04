#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConsts {
    float scale;
} pc;

void main() {
    gl_Position = vec4(inPosition / pc.scale, 0.0, 1.0);
    fragColor = vec4(1, 1, 0.0, 0.0);
}