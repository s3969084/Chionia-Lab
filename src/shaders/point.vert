
#version 450

layout(location = 0) in uint id;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inTargetPosition;
layout(location = 3) in vec4 inPointColor;
layout(location = 4) in vec3 inOrbitCenter;
layout(location = 5) in vec3 inOrbitAxis;
layout(location = 6) in float inOrbitAngularSpeed;
layout(location = 7) in vec3 inSecondaryOrbitCenter;
layout(location = 8) in vec3 inSecondaryOrbitAxis;
layout(location = 9) in float inSecondaryOrbitAngularSpeed;
layout(location = 10) in vec3 inSpinCenter;
layout(location = 11) in vec3 inSpinAxis;
layout(location = 12) in float inSpinAngularSpeed;
layout(location = 13) in vec4 inTargetPointColor;

layout(location = 0) flat out float outPulse;
layout(location = 1) flat out float outTrailAngle;
layout(location = 2) flat out float outPalettePosition;
layout(location = 3) flat out float outReflection;
layout(location = 4) flat out float outReflectionStrength;
layout(location = 5) flat out vec4 outPointColor;
layout(location = 6) flat out float outEnergyEffectsEnabled;
layout(location = 7) flat out float outSunBody;
layout(location = 8) flat out float outSolarOcclusion;

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

vec3 rotateAroundAxis(
    vec3 position,
    vec3 center,
    vec3 axis,
    float angle
) {
    vec3 normalizedAxis = normalize(axis);
    vec3 relativePosition = position - center;
    float cosine = cos(angle);
    float sine = sin(angle);

    return center +
        relativePosition * cosine +
        cross(normalizedAxis, relativePosition) * sine +
        normalizedAxis * dot(normalizedAxis, relativePosition) * (1.0 - cosine);
}

void main() {

    bool isReflection = (id & 0x80000000u) != 0u;
    uint particleId = id & 0x7fffffffu;

    float elapsedTime = ubo.animation.x;
    float amplitude = ubo.animation.y;
    float speed = ubo.animation.z;
    float vibrationEnabled = ubo.animation.w;
    float energyEffectsEnabled = ubo.morph.z;

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

    float orbitalBlend = smoothstep(0.90, 1.0, morphProgress);
    vec3 secondaryOrbitCenter = inSecondaryOrbitCenter;
    vec3 spinCenter = inSpinCenter;
    vec3 spinAxis = normalize(inSpinAxis);
    if (abs(inOrbitAngularSpeed) > 0.0001) {
        float orbitAngle = ubo.morph.w * inOrbitAngularSpeed;
        vec3 rotatedPosition = rotateAroundAxis(
            basePosition,
            inOrbitCenter,
            inOrbitAxis,
            orbitAngle
        );
        vec3 rotatedSecondaryCenter = rotateAroundAxis(
            secondaryOrbitCenter,
            inOrbitCenter,
            inOrbitAxis,
            orbitAngle
        );
        secondaryOrbitCenter = mix(
            secondaryOrbitCenter,
            rotatedSecondaryCenter,
            orbitalBlend
        );
        vec3 rotatedSpinCenter = rotateAroundAxis(
            spinCenter,
            inOrbitCenter,
            inOrbitAxis,
            orbitAngle
        );
        spinCenter = mix(spinCenter, rotatedSpinCenter, orbitalBlend);
        vec3 rotatedSpinAxis = rotateAroundAxis(
            spinAxis,
            vec3(0.0),
            inOrbitAxis,
            orbitAngle
        );
        spinAxis = normalize(mix(spinAxis, rotatedSpinAxis, orbitalBlend));
        basePosition = mix(basePosition, rotatedPosition, orbitalBlend);
    }
    if (abs(inSecondaryOrbitAngularSpeed) > 0.0001) {
        vec3 secondaryRotatedPosition = rotateAroundAxis(
            basePosition,
            secondaryOrbitCenter,
            inSecondaryOrbitAxis,
            ubo.morph.w * inSecondaryOrbitAngularSpeed
        );
        basePosition = mix(
            basePosition,
            secondaryRotatedPosition,
            orbitalBlend
        );
    }
    if (abs(inSpinAngularSpeed) > 0.0001) {
        vec3 spunPosition = rotateAroundAxis(
            basePosition,
            spinCenter,
            spinAxis,
            ubo.morph.w * inSpinAngularSpeed
        );
        basePosition = mix(basePosition, spunPosition, orbitalBlend);
    }

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

    // Keep particle brightness and size steady; object motion remains animated.
    outPulse = 0.5;

    // --------------------------------------------------
    // ENERGY TRAIL DIRECTION
    // --------------------------------------------------

    // Give each particle a slightly different streak angle.
    outTrailAngle = energyEffectsEnabled > 0.5
        ? hash(particleId + 23u) * 6.28318530718
        : 0.0;

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

    float solarOcclusion = 0.0;
    bool isOrbitingBody =
        length(inSpinCenter) > 0.1 &&
        abs(inOrbitAngularSpeed) > 0.0001;
    if (isOrbitingBody) {
        const float sunRadius = 0.29025;
        vec4 sunViewPosition =
            ubo.view * ubo.model * vec4(0.0, 0.0, 0.0, 1.0);
        vec4 bodyViewPosition = ubo.view * worldPosition;
        vec2 sunPlaneOffset = bodyViewPosition.xy - sunViewPosition.xy;
        float sunPlaneDistanceSquared = dot(sunPlaneOffset, sunPlaneOffset);
        if (sunPlaneDistanceSquared < sunRadius * sunRadius) {
            float sunFrontDistance = -sunViewPosition.z - sqrt(
                max(
                    sunRadius * sunRadius - sunPlaneDistanceSquared,
                    0.0
                )
            );
            float bodyDistance = -bodyViewPosition.z;
            solarOcclusion = bodyDistance < sunFrontDistance ? 1.0 : -1.0;
        }
    }

    gl_Position = ubo.projection * ubo.view * worldPosition;
    if (isReflection && !drawReflection) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    }

    outPalettePosition = energyEffectsEnabled > 0.5
        ? clamp(animatedPosition.x + 0.5, 0.0, 1.0)
        : 0.5;
    outPointColor = mix(inPointColor, inTargetPointColor, progress);
    outEnergyEffectsEnabled = energyEffectsEnabled;
    outSunBody =
        length(inSpinCenter) < 0.01 &&
        abs(inSpinAngularSpeed) > 0.0001
            ? 1.0
            : 0.0;
    outSolarOcclusion = solarOcclusion;
    outReflection = drawReflection ? 1.0 : 0.0;
    outReflectionStrength = drawReflection
        ? 0.38 *
            exp(-max(heightAboveGround, 0.0) * 2.0) *
            exp(-ubo.ground.z * 3.0)
        : 0.0;

    gl_PointSize = energyEffectsEnabled > 0.5
        ? 6.9 * mix(1.0, 0.76, outReflection)
        : 4.5 * mix(1.0, 0.76, outReflection);
}
