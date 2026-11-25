
#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inTargetPosition;

layout(location = 0) flat out float outBreathing;
layout(location = 1) flat out float outPulse;
layout(location = 2) flat out float outSpark;
layout(location = 3) flat out float outTrailAngle;
layout(location = 4) flat out float outPalettePosition;
layout(location = 5) flat out float outReflection;
layout(location = 6) flat out float outReflectionStrength;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 animation;
    vec4 morph;
    vec4 ground;
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

    bool isReflection = (id & 0x80000000u) != 0u;
    uint particleId = id & 0x7fffffffu;

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
        hash(particleId + 1u),
        hash(particleId + 2u),
        hash(particleId + 3u)
    ) * 2.0 - 1.0;

    float directionLength = length(randomDirection);

    vec3 direction = directionLength > 0.0001
        ? randomDirection / directionLength
        : vec3(0.0, 1.0, 0.0);

    float vibrationPhase =
        hash(particleId + 4u) * 6.28318530718;

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
        hash(particleId + 5u) * 0.5;

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
    float sparkSeed = hash(particleId + 19u);

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
        hash(particleId + 23u) * 6.28318530718;

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

    vec4 worldPosition = ubo.model * vec4(animatedPosition, 1.0);
    float heightAboveGround = worldPosition.y - ubo.ground.x;
    bool drawReflection =
        isReflection &&
        ubo.ground.y > 0.5 &&
        heightAboveGround >= -0.001;

    if (drawReflection) {
        float reflectionDistance = max(heightAboveGround, 0.0);
        float ripplePhase =
            (worldPosition.x + worldPosition.z) * 11.0 - elapsedTime * 2.4;
        float rippleWeight = smoothstep(0.0, 0.7, reflectionDistance);
        float ripple = sin(ripplePhase) * 0.008 * rippleWeight;

        worldPosition.y = 2.0 * ubo.ground.x - worldPosition.y;
        worldPosition.x += ripple;
        worldPosition.z += cos(ripplePhase * 0.8) * 0.006 * rippleWeight;
    }

    gl_Position = ubo.projection * ubo.view * worldPosition;
    if (isReflection && !drawReflection) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }

    outPalettePosition = clamp(animatedPosition.x + 0.5, 0.0, 1.0);
    outReflection = drawReflection ? 1.0 : 0.0;
    outReflectionStrength = drawReflection
        ? 0.38 *
            exp(-max(heightAboveGround, 0.0) * 2.0) *
            exp(-ubo.ground.z * 3.0)
        : 0.0;

    gl_PointSize = mix(
        6.5,
        9.5,
        pulseWave
    ) * mix(1.0, 0.76, outReflection);
}
