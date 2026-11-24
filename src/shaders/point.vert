
#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inTargetPosition;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 animation;
    vec4 morph;
} ubo;

// Deterministic hash for each particle.
float hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;

    return float(x) / 4294967295.0;
}

void main() {

    // Existing vibration parameters.
    float elapsedTime = ubo.animation.x;
    float amplitude = ubo.animation.y;
    float speed = ubo.animation.z;
    float vibrationEnabled = ubo.animation.w;

    // Morph parameters.
    float morphProgress = clamp(ubo.morph.x, 0.0, 1.0);
    float morphEnabled = ubo.morph.y;

    // Smooth interpolation between starting and destination positions.
    float smoothProgress =
        morphProgress * morphProgress * (3.0 - 2.0 * morphProgress);

    float progress = mix(0.0, smoothProgress, morphEnabled);

    vec3 basePosition = mix(
        inPosition,
        inTargetPosition,
        progress
    );

    // Generate a unique direction and phase for each particle.
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

    // Preserve existing particle vibration.
    float vibration = sin(elapsedTime * speed + phase);

    vec3 animatedPosition =
        basePosition +
        direction * vibration * amplitude * vibrationEnabled;

    gl_Position = ubo.projection *
                  ubo.view *
                  ubo.model *
                  vec4(animatedPosition, 1.0);

    gl_PointSize = 1.0;
}
