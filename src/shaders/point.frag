
#version 450

layout(location = 0) flat in float inPulse;
layout(location = 1) flat in float inTrailAngle;
layout(location = 2) flat in float inPalettePosition;
layout(location = 3) flat in float inReflection;
layout(location = 4) flat in float inReflectionStrength;
layout(location = 5) flat in vec4 inPointColor;
layout(location = 6) flat in float inEnergyEffectsEnabled;
layout(location = 7) flat in float inSunBody;
layout(location = 8) flat in float inSolarOcclusion;

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

    vec3 electricBlue = vec3(
        0.08,
        0.32,
        1.0
    );

    vec3 plasmaViolet = vec3(
        0.57,
        0.18,
        1.0
    );

    // Keep the highlight cool and tinted so bright cores retain their colour.
    vec3 softWhite = vec3(
        0.62,
        0.80,
        0.96
    );

    // --------------------------------------------------
    // TRAVELLING COLOUR TRANSITION
    // --------------------------------------------------

    vec3 particleColor = inPointColor.rgb;
    if (inEnergyEffectsEnabled > 0.5) {
        particleColor = mix(
            cyan,
            electricBlue,
            smoothstep(0.05, 0.55, inPalettePosition)
        );

        particleColor = mix(
            particleColor,
            plasmaViolet,
            smoothstep(0.48, 0.98, inPalettePosition)
        );

        particleColor = mix(
            particleColor,
            iceBlue,
            smoothstep(0.35, 0.95, inPulse) * 0.22
        );

        particleColor = mix(
            particleColor,
            softWhite,
            core * 0.03
        );

        float customColorAmount = smoothstep(
            0.02,
            0.18,
            length(vec3(1.0) - inPointColor.rgb)
        );
        particleColor = mix(
            particleColor,
            mix(particleColor, inPointColor.rgb, 0.82),
            customColorAmount
        );
    }

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
    // LIGHT COMPOSITION
    // --------------------------------------------------

    float intensity = inEnergyEffectsEnabled > 0.5
        ? core * 0.52 + halo * 0.09
        : core;

    vec3 finalColor =
        particleColor *
        intensity;

    finalColor *= mix(
        vec3(1.0),
        vec3(0.30, 0.68, 0.95),
        inReflection
    );

    // Energy trail.
    if (inEnergyEffectsEnabled > 0.5) {
        finalColor +=
            iceBlue *
            trailMask *
            trailIntensity *
            0.18;

        finalColor +=
            iceBlue *
            rippleBand *
            0.10;

        // The Vulkan particle pass is additive, so dense peaks accumulate
        // quickly. Leave headroom here to keep those cores saturated in color.
        finalColor *= 0.35;
    }

    // Keep the dense solar point cloud translucent enough for nearby planets
    // to remain legible in the additive particle renderer.
    finalColor *= mix(1.0, 0.58, inSunBody);
    if (inSolarOcclusion > 0.5) {
        finalColor *= 1.8;
    }
    else if (inSolarOcclusion < -0.5) {
        finalColor *= 0.12;
    }

    // --------------------------------------------------
    // TRANSPARENCY
    // --------------------------------------------------

    float alpha = inEnergyEffectsEnabled > 0.5
        ? clamp(
            core * 0.75 +
            halo * 0.12 +
            trailMask * trailIntensity * 0.10 +
            rippleBand * 0.05,
            0.0,
            0.85
        )
        : core * 0.82;

    finalColor *= mix(1.0, inReflectionStrength, inReflection);
    alpha *= mix(1.0, inReflectionStrength, inReflection);
    alpha *= inPointColor.a;

    outColor = vec4(
        finalColor,
        alpha
    );
}
