#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;

// UBO binding (set = 0, binding = 0)
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 animation;
} ubo;

// Deterministic hash: generates a repeatable value for each point.
float hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;

    return float(x) / 4294967295.0;
}

void main() {

    // Animation parameters
    float elapsedTime = ubo.animation.x;
    float amplitude  = ubo.animation.y;
    float speed      = ubo.animation.z;
    float enabled    = ubo.animation.w;

    // Generate a unique direction and phase for each point.
    vec3 randomDirection = vec3(
        hash(id + 1u),
        hash(id + 2u),
        hash(id + 3u)
    ) * 2.0 - 1.0;

    float directionLength = length(randomDirection);

    vec3 direction = directionLength > 0.0001
        ? randomDirection / directionLength
        : vec3(0.0, 1.0, 0.0);

    float phase = hash(id + 4u) * 6.28318530718;

    // Calculate independent vibration.
    float vibration = sin(elapsedTime * speed + phase);

    vec3 animatedPosition =
        inPosition + direction * vibration * amplitude * enabled;

    // Preserve the original transformation pipeline.
    gl_Position = ubo.projection *
                  ubo.view *
                  ubo.model *
                  vec4(animatedPosition, 1.0);

    gl_PointSize = 1.0;
}