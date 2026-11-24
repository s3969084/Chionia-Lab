
#version 450

layout(location = 0) flat in float inBreathing;
layout(location = 1) flat in float inPulse;
layout(location = 2) flat in float inSpark;
layout(location = 3) flat in float inTrailAngle;

layout(location = 0) out vec4 outColor;

void main() {

    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dist = length(coord);

    // --------------------------------------------------
    // PARTICLE CORE
    // --------------------------------------------------

    float core = 1.0 - smoothstep(
        0.05,
        0.42,
        dist
    );

    // --------------------------------------------------
    // AMBIENT HALO
    // --------------------------------------------------

    float halo = exp(-dist * 5.0) *
                 (1.0 - smoothstep(0.45, 1.0, dist));

    // --------------------------------------------------
    // CHIONIA COLOUR PALETTE
    // --------------------------------------------------

    vec3 cyan = vec3(
        0.0,
        0.851,
        1.0
    );

    vec3 iceBlue = vec3(
        0.55,
        0.91,
        1.0
    );

    vec3 plasmaGold = vec3(
        1.0,
        0.72,
        0.32
    );

    vec3 softWhite = vec3(
        0.95,
        0.98,
        1.0
    );

    // --------------------------------------------------
    // TRAVELLING COLOUR TRANSITION
    // --------------------------------------------------

    vec3 particleColor = mix(
        cyan,
        iceBlue,
        smoothstep(0.20, 0.80, inPulse)
    );

    float goldAmount = smoothstep(
        0.82,
        0.98,
        inPulse
    );

    particleColor = mix(
        particleColor,
        plasmaGold,
        goldAmount * 0.85
    );

    particleColor = mix(
        particleColor,
        softWhite,
        core * 0.18
    );

    // --------------------------------------------------
    // DIRECTIONAL ENERGY TRAIL
    // --------------------------------------------------

    float c = cos(inTrailAngle);
    float s = sin(inTrailAngle);

    mat2 rotation = mat2(
        c, -s,
        s,  c
    );

    vec2 trailCoord = rotation * coord;

    float trailWidth = mix(
        0.08,
        0.015,
        clamp(-trailCoord.x, 0.0, 1.0)
    );

    float trailMask =
        smoothstep(-0.95, -0.25, trailCoord.x) *
        (1.0 - smoothstep(-0.10, 0.08, trailCoord.x)) *
        (1.0 - smoothstep(
            trailWidth,
            trailWidth * 2.0,
            abs(trailCoord.y)
        ));

    float trailIntensity = mix(
        0.12,
        0.65,
        inPulse
    );

    // --------------------------------------------------
    // ENERGY RIPPLE
    // --------------------------------------------------

    float rippleRadius = mix(
        0.52,
        0.68,
        inPulse
    );

    float rippleBand = exp(
        -pow((dist - rippleRadius) * 14.0, 2.0)
    );

    rippleBand *= smoothstep(
        0.65,
        0.98,
        inPulse
    );

    // --------------------------------------------------
    // PLASMA SPARK
    // --------------------------------------------------

    float sparkCore = inSpark *
        (1.0 - smoothstep(0.0, 0.25, dist));

    float sparkHalo = inSpark *
        exp(-dist * 7.0);

    // --------------------------------------------------
    // LIGHT COMPOSITION
    // --------------------------------------------------

    float intensity =
        core * 0.85 +
        halo * 0.20;

    vec3 finalColor =
        particleColor *
        intensity *
        inBreathing;

    // Energy trail.
    finalColor +=
        iceBlue *
        trailMask *
        trailIntensity *
        0.18;

    // Ripple highlight.
    finalColor +=
        iceBlue *
        rippleBand *
        0.10;

    // Plasma-gold spark.
    finalColor +=
        plasmaGold *
        sparkHalo *
        0.28;

    // Hot white spark centre.
    finalColor +=
        softWhite *
        sparkCore *
        0.55;

    // --------------------------------------------------
    // TRANSPARENCY
    // --------------------------------------------------

    float alpha = clamp(
        core * 0.75 +
        halo * 0.12 +
        trailMask * trailIntensity * 0.10 +
        rippleBand * 0.05 +
        inSpark * 0.12,
        0.0,
        0.85
    );

    outColor = vec4(
        finalColor,
        alpha
    );
}
