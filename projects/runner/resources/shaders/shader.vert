#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConsts {
    vec4 color;
    mat2 transform;
    vec2 position;
} pc;

void main() {
    gl_Position = vec4(pc.transform * inPosition + pc.position, 0.0, 1.0);
    fragColor = pc.color;
}