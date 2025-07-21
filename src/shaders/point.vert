#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;

// UBO binding (set = 0, binding = 0)
layout(set = 0, binding = 0) uniform UniformBufferObject {
mat4 model;
mat4 view;
mat4 projection;
} ubo;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_PointSize = 10.0;
}
