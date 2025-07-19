#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    gl_PointSize = 10.0;
}
