
#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inTargetPosition;

layout(location = 0) flat out float outBreathing;
layout(location = 1) flat out float outPulse;
layout(location = 2) flat out float outSpark;
layout(location = 3) flat out float outTrailAngle;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 animation;
    vec4 morph;
} ubo;

float hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;

    return float(x) / 4294967295.0;
}

void main() {

    float elapsedTime = ubo.animation.x;
    float amplitude = ubo.animation.y;
    float speed = ubo.animation.z;
    float vibrationEnabled = ubo.animation.w;

    // --------------------------------------------------
    // SHAPE MORPHING
    // --------------------------------------------------

    float morphProgress = clamp(ubo.morph.x, 0.0, 1.0);
    float morphEnabled = ubo.morph.y;

    float smoothProgress =
        morphProgress * morphProgress *
        (3.0 - 2.0 * morphProgress);

    float progress = mix(
        0.0,
        smoothProgress,
        morphEnabled
    );

    vec3 basePosition = mix(
        inPosition,
        inTargetPosition,
        progress
    );

    // --------------------------------------------------
    // PARTICLE VIBRATION
    // --------------------------------------------------

    vec3 randomDirection = vec3(
        hash(id + 1u),
        hash(id + 2u),
        hash(id + 3u)
    ) * 2.0 - 1.0;

    float directionLength = length(randomDirection);

    vec3 direction = directionLength > 0.0001
        ? randomDirection / directionLength
        : vec3(0.0, 1.0, 0.0);

    float vibrationPhase =
        hash(id + 4u) * 6.28318530718;

    float vibration = sin(
        elapsedTime * speed + vibrationPhase
    );

    vec3 animatedPosition =
        basePosition +
        direction * vibration * amplitude * vibrationEnabled;

    // --------------------------------------------------
    // TRAVELLING ENERGY PULSE
    // --------------------------------------------------

    float pulseSpeed = 2.0;
    float pulseFrequency = 3.0;

    float spatialPhase =
        length(basePosition) * pulseFrequency;

    float particlePhase =
        hash(id + 5u) * 0.5;

    float pulseWave = 0.5 + 0.5 * sin(
        elapsedTime * pulseSpeed -
        spatialPhase +
        particlePhase
    );

    outPulse = pulseWave;

    // --------------------------------------------------
    // PLASMA SPARK GENERATION
    // --------------------------------------------------

    // Only a small percentage of particles can spark.
    float sparkSeed = hash(id + 19u);

    float sparkEligibility = smoothstep(
        0.86,
        0.98,
        sparkSeed
    );

    // Sparks activate near the pulse peak.
    float sparkEnvelope = smoothstep(
        0.90,
        0.99,
        pulseWave
    );

    outSpark = sparkEligibility * sparkEnvelope;

    // --------------------------------------------------
    // ENERGY TRAIL DIRECTION
    // --------------------------------------------------

    // Give each particle a slightly different streak angle.
    outTrailAngle =
        hash(id + 23u) * 6.28318530718;

    // --------------------------------------------------
    // BREATHING / ENERGY INTENSITY
    // --------------------------------------------------

    outBreathing = mix(
        0.72,
        1.0,
        smoothstep(0.25, 0.85, pulseWave)
    );

    // --------------------------------------------------
    // PROJECTION
    // --------------------------------------------------

    gl_Position = ubo.projection *
                  ubo.view *
                  ubo.model *
                  vec4(animatedPosition, 1.0);

    gl_PointSize = mix(
        8.5,
        12.5,
        pulseWave
    );
}
