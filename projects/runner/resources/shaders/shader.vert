#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UBO {
    vec4 color;
    float scale;
} ubo;


layout(location = 0) in vec2 inPosition;
layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPosition / ubo.scale, 0.0, 1.0);
    fragColor = ubo.color;
}