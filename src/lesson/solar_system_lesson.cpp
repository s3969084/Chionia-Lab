#include "solar_system_lesson.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace chionia {

    namespace {

        constexpr float PI = 3.14159265359f;
        constexpr float GOLDEN_ANGLE = 2.39996322973f;
        constexpr float ORBIT_PLANE_HEIGHT = 0.0f;
        constexpr float SUN_RADIUS = 0.135f;
        constexpr float PLUTO_ORBIT = 0.70f;
        constexpr float PLUTO_SPEED = 0.085f;
        constexpr float MOON_ORBIT_RADIUS = 0.055f;
        constexpr float MOON_ORBIT_SPEED = 1.6f;
        constexpr float ORBIT_SPEED_SCALE = 0.10f;
        constexpr float SUN_SPIN_SPEED = 0.035f;
        constexpr std::size_t LESSON_POINT_COUNT = MORPH_PARTICLE_COUNT;
        constexpr float LESSON_SCENE_SCALE = 2.15f;
        constexpr float LESSON_ORBIT_TILT = 1.15f;
        constexpr std::size_t PLANET_SURFACE_POINT_MULTIPLIER = 8;

        enum class SurfaceStyle {
            Sun,
            Mercury,
            Venus,
            Earth,
            Mars,
            Jupiter,
            Saturn,
            Uranus,
            Neptune,
            Moon,
            Pluto
        };

        struct Planet {
            const char* name;
            float orbitRadius;
            float orbitalPhase;
            float orbitalSpeed;
            float radius;
            float spinSpeed;
            float axialTilt;
            glm::vec4 color;
            std::size_t pointCount;
            SurfaceStyle surfaceStyle;
        };

        constexpr std::array<Planet, 8> PLANETS = {{
            {"Mercury", 0.14f, 0.30f, 0.90f, 0.019f, 0.045f, 0.034f, {0.58f, 0.56f, 0.52f, 1.0f}, 360, SurfaceStyle::Mercury},
            {"Venus",   0.20f, 2.60f, 0.70f, 0.026f, 0.018f, 177.4f, {0.86f, 0.72f, 0.49f, 1.0f}, 480, SurfaceStyle::Venus},
            {"Earth",   0.27f, 4.30f, 0.55f, 0.030f, 0.070f, 23.4f, {0.12f, 0.34f, 0.76f, 1.0f}, 640, SurfaceStyle::Earth},
            {"Mars",    0.34f, 0.90f, 0.45f, 0.023f, 0.065f, 25.2f, {0.74f, 0.29f, 0.16f, 1.0f}, 520, SurfaceStyle::Mars},
            {"Jupiter", 0.43f, 3.55f, 0.25f, 0.074f, 0.16f, 3.1f, {0.72f, 0.53f, 0.37f, 1.0f}, 1240, SurfaceStyle::Jupiter},
            {"Saturn",  0.51f, 5.45f, 0.18f, 0.066f, 0.14f, 26.7f, {0.82f, 0.74f, 0.57f, 1.0f}, 1120, SurfaceStyle::Saturn},
            {"Uranus",  0.58f, 1.90f, 0.12f, 0.047f, 0.075f, 97.8f, {0.47f, 0.75f, 0.79f, 1.0f}, 920, SurfaceStyle::Uranus},
            {"Neptune", 0.64f, 4.85f, 0.09f, 0.046f, 0.080f, 28.3f, {0.19f, 0.34f, 0.78f, 1.0f}, 920, SurfaceStyle::Neptune}
        }};

        constexpr glm::vec4 MOON_COLOR(0.70f, 0.69f, 0.65f, 1.0f);
        constexpr glm::vec4 PLUTO_COLOR(0.67f, 0.56f, 0.46f, 1.0f);

        glm::vec3 tiltDirection(const glm::vec3& direction) {
            const float cosine = std::cos(LESSON_ORBIT_TILT);
            const float sine = std::sin(LESSON_ORBIT_TILT);
            return glm::vec3(
                direction.x,
                direction.y * cosine - direction.z * sine,
                direction.y * sine + direction.z * cosine
            );
        }

        glm::vec3 framePosition(const glm::vec3& position) {
            return tiltDirection(position) * LESSON_SCENE_SCALE;
        }

        PointOrbitMotion frameOrbitMotion(const PointOrbitMotion& motion) {
            PointOrbitMotion framed = motion;
            framed.center = framePosition(motion.center);
            framed.axis = tiltDirection(motion.axis);
            framed.secondaryCenter = framePosition(motion.secondaryCenter);
            framed.secondaryAxis = tiltDirection(motion.secondaryAxis);
            framed.spinCenter = framePosition(motion.spinCenter);
            framed.spinAxis = tiltDirection(motion.spinAxis);
            return framed;
        }

        glm::vec3 orbitPosition(float radius, float angle) {
            return glm::vec3(
                std::cos(angle) * radius,
                ORBIT_PLANE_HEIGHT + std::sin(angle) * radius,
                0.0f
            );
        }

        glm::vec3 planetPosition(const Planet& planet, float elapsedTime) {
            return orbitPosition(
                planet.orbitRadius,
                planet.orbitalPhase +
                    planet.orbitalSpeed * ORBIT_SPEED_SCALE * elapsedTime
            );
        }

        PointOrbitMotion orbitAroundSun(float angularSpeed) {
            PointOrbitMotion motion;
            motion.center = glm::vec3(0.0f, ORBIT_PLANE_HEIGHT, 0.0f);
            motion.axis = glm::vec3(0.0f, 0.0f, 1.0f);
            motion.angularSpeed = angularSpeed * ORBIT_SPEED_SCALE;
            return motion;
        }

        PointOrbitMotion spinningBodyMotion(
            const glm::vec3& center,
            float orbitalSpeed,
            float spinSpeed,
            float axialTiltDegrees = 0.0f
        ) {
            PointOrbitMotion motion = orbitAroundSun(orbitalSpeed);
            motion.spinCenter = center;
            const float tiltRadians = axialTiltDegrees * PI / 180.0f;
            motion.spinAxis = glm::vec3(
                std::sin(tiltRadians),
                0.0f,
                std::cos(tiltRadians)
            );
            motion.spinAngularSpeed = spinSpeed;
            return motion;
        }

        float latticeHash(const glm::vec3& cell) {
            const float value = std::sin(glm::dot(
                cell,
                glm::vec3(127.1f, 311.7f, 74.7f)
            )) * 43758.5453f;
            return value - std::floor(value);
        }

        float valueNoise(const glm::vec3& position) {
            const glm::vec3 cell = glm::floor(position);
            glm::vec3 local = glm::fract(position);
            local = local * local * (glm::vec3(3.0f) - 2.0f * local);

            const float x00 = glm::mix(
                latticeHash(cell),
                latticeHash(cell + glm::vec3(1.0f, 0.0f, 0.0f)),
                local.x
            );
            const float x10 = glm::mix(
                latticeHash(cell + glm::vec3(0.0f, 1.0f, 0.0f)),
                latticeHash(cell + glm::vec3(1.0f, 1.0f, 0.0f)),
                local.x
            );
            const float x01 = glm::mix(
                latticeHash(cell + glm::vec3(0.0f, 0.0f, 1.0f)),
                latticeHash(cell + glm::vec3(1.0f, 0.0f, 1.0f)),
                local.x
            );
            const float x11 = glm::mix(
                latticeHash(cell + glm::vec3(0.0f, 1.0f, 1.0f)),
                latticeHash(cell + glm::vec3(1.0f, 1.0f, 1.0f)),
                local.x
            );
            const float y0 = glm::mix(x00, x10, local.y);
            const float y1 = glm::mix(x01, x11, local.y);
            return glm::mix(y0, y1, local.z);
        }

        float surfaceNoise(glm::vec3 position) {
            float total = 0.0f;
            float weight = 0.5f;
            float weightSum = 0.0f;
            for (int octave = 0; octave < 4; ++octave) {
                total += valueNoise(position) * weight;
                weightSum += weight;
                position = position * 2.0f + glm::vec3(13.7f, 29.3f, 7.1f);
                weight *= 0.5f;
            }
            return total / weightSum;
        }

        float pointNoise(std::size_t pointIndex) {
            std::uint32_t value = static_cast<std::uint32_t>(pointIndex) +
                0x9e3779b9u;
            value ^= value >> 16u;
            value *= 0x7feb352du;
            value ^= value >> 15u;
            value *= 0x846ca68bu;
            value ^= value >> 16u;
            return static_cast<float>(value & 0x00ffffffu) /
                static_cast<float>(0x01000000u);
        }

        glm::vec3 applyPlanetLighting(
            const glm::vec3& color,
            const glm::vec3& normal,
            const glm::vec3& lightDirection
        ) {
            // Add a broad board-facing fill light so the Earth-side detail
            // remains readable while the Sun still gives each globe a lit side.
            const glm::vec3 cameraFillDirection = glm::normalize(glm::vec3(
                0.0f,
                std::sin(LESSON_ORBIT_TILT),
                std::cos(LESSON_ORBIT_TILT)
            ));
            const glm::vec3 combinedLightDirection = glm::normalize(
                lightDirection * 0.65f + cameraFillDirection * 1.15f
            );
            const float diffuse = std::max(
                0.0f,
                glm::dot(normal, combinedLightDirection)
            );
            const float light = 0.42f + 0.58f * std::sqrt(diffuse);
            return color * light;
        }

        glm::vec4 surfaceColor(
            SurfaceStyle style,
            const glm::vec4& baseColor,
            const glm::vec3& direction,
            const glm::vec3& spinAxis,
            const glm::vec3& lightDirection,
            std::size_t pointIndex
        ) {
            const float latitude = glm::dot(direction, spinAxis);
            const float longitude = std::atan2(
                direction.z,
                direction.x
            );
            const float broadNoise = surfaceNoise(
                direction * 3.5f + glm::vec3(1.7f, 8.2f, 3.4f)
            );
            const float fineNoise = surfaceNoise(
                direction * 23.0f + glm::vec3(4.3f, 1.1f, 9.7f)
            );
            const float grain = pointNoise(pointIndex);
            glm::vec3 color(baseColor);

            switch (style) {
                case SurfaceStyle::Sun: {
                    const float granulation = surfaceNoise(
                        direction * 38.0f + glm::vec3(12.0f, 4.0f, 27.0f)
                    );
                    color = glm::mix(
                        glm::vec3(0.94f, 0.43f, 0.08f),
                        glm::vec3(1.0f, 0.83f, 0.34f),
                        glm::smoothstep(0.25f, 0.78f, broadNoise)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(1.0f, 0.96f, 0.71f),
                        glm::smoothstep(0.58f, 0.88f, granulation) * 0.45f
                    );
                    color *= 0.94f + grain * 0.12f;
                    break;
                }
                case SurfaceStyle::Mercury: {
                    const float craterField = surfaceNoise(
                        direction * 18.0f + glm::vec3(8.0f, 23.0f, 3.0f)
                    );
                    const float ejectaRim = 1.0f - std::abs(
                        surfaceNoise(
                            direction * 31.0f + glm::vec3(31.0f, 6.0f, 15.0f)
                        ) - 0.5f
                    ) * 2.0f;
                    color = glm::mix(
                        glm::vec3(0.20f, 0.20f, 0.21f),
                        glm::vec3(0.67f, 0.64f, 0.59f),
                        glm::smoothstep(0.30f, 0.75f, broadNoise)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(0.79f, 0.74f, 0.65f),
                        glm::smoothstep(0.73f, 0.89f, craterField) *
                            glm::smoothstep(0.30f, 0.76f, ejectaRim) * 0.42f
                    );
                    color *= 0.90f + fineNoise * 0.20f;
                    break;
                }
                case SurfaceStyle::Venus: {
                    const float cloudWarp = surfaceNoise(
                        direction * 8.0f + glm::vec3(7.0f, 11.0f, 19.0f)
                    );
                    const float cloudBand = std::sin(
                        latitude * 17.0f + std::sin(longitude * 3.0f) * 0.8f +
                        (cloudWarp - 0.5f) * 3.0f
                    );
                    color = glm::mix(
                        glm::vec3(0.56f, 0.35f, 0.17f),
                        glm::vec3(0.98f, 0.82f, 0.52f),
                        glm::smoothstep(-0.55f, 0.62f, cloudBand)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(1.0f, 0.91f, 0.70f),
                        glm::smoothstep(0.65f, 0.85f, fineNoise) * 0.35f
                    );
                    break;
                }
                case SurfaceStyle::Earth: {
                    const float elevation = surfaceNoise(
                        direction * 4.2f + glm::vec3(2.0f, 6.0f, 1.0f)
                    ) + (broadNoise - 0.5f) * 0.18f;
                    const float land = glm::smoothstep(0.47f, 0.55f, elevation);
                    const float dryRegions = surfaceNoise(
                        direction * 6.0f + glm::vec3(21.0f, 4.0f, 12.0f)
                    );
                    const glm::vec3 ocean = glm::mix(
                        glm::vec3(0.018f, 0.085f, 0.26f),
                        glm::vec3(0.035f, 0.30f, 0.55f),
                        glm::smoothstep(0.38f, 0.69f, fineNoise)
                    );
                    const glm::vec3 greenLand = glm::mix(
                        glm::vec3(0.075f, 0.25f, 0.12f),
                        glm::vec3(0.30f, 0.39f, 0.19f),
                        glm::smoothstep(0.34f, 0.70f, fineNoise)
                    );
                    const glm::vec3 dryLand(0.55f, 0.39f, 0.20f);
                    const glm::vec3 landColor = glm::mix(
                        greenLand,
                        dryLand,
                        glm::smoothstep(0.59f, 0.78f, dryRegions)
                    );
                    color = glm::mix(ocean, landColor, land);
                    color = glm::mix(
                        color,
                        glm::vec3(0.57f, 0.48f, 0.36f),
                        (1.0f - glm::smoothstep(0.53f, 0.59f, elevation)) * land * 0.42f
                    );
                    const float cloudCoverage = surfaceNoise(
                        direction * 11.0f + glm::vec3(18.0f, 5.0f, 25.0f)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(0.89f, 0.94f, 0.98f),
                        glm::smoothstep(0.61f, 0.76f, cloudCoverage) * 0.78f
                    );
                    if (std::abs(latitude) > 0.91f) {
                        color = glm::mix(
                            color,
                            glm::vec3(0.86f, 0.92f, 0.97f),
                            glm::smoothstep(0.91f, 0.98f, std::abs(latitude)) * 0.92f
                        );
                    }
                    break;
                }
                case SurfaceStyle::Mars: {
                    const float mineral = surfaceNoise(
                        direction * 9.0f + glm::vec3(22.0f, 3.0f, 8.0f)
                    );
                    const float darkTerrain = surfaceNoise(
                        direction * 5.0f + glm::vec3(4.0f, 19.0f, 13.0f)
                    );
                    color = glm::mix(
                        glm::vec3(0.36f, 0.105f, 0.047f),
                        glm::vec3(0.86f, 0.37f, 0.15f),
                        glm::smoothstep(0.27f, 0.77f, mineral)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(0.20f, 0.16f, 0.13f),
                        glm::smoothstep(0.62f, 0.77f, darkTerrain) * 0.58f
                    );
                    if (std::abs(latitude) > 0.91f) {
                        color = glm::mix(
                            color,
                            glm::vec3(0.89f, 0.84f, 0.74f),
                            glm::smoothstep(0.91f, 0.985f, std::abs(latitude)) * 0.88f
                        );
                    }
                    break;
                }
                case SurfaceStyle::Jupiter:
                case SurfaceStyle::Saturn: {
                    const float turbulence = surfaceNoise(
                        direction * 7.0f + glm::vec3(3.0f, 16.0f, 10.0f)
                    );
                    const float bandRate = style == SurfaceStyle::Jupiter
                        ? 28.0f
                        : 20.0f;
                    const float bands = std::sin(
                        latitude * bandRate + std::sin(longitude * 2.2f) * 0.7f +
                        (turbulence - 0.5f) * 2.6f
                    );
                    const glm::vec3 darkBand = style == SurfaceStyle::Jupiter
                        ? glm::vec3(0.43f, 0.23f, 0.13f)
                        : glm::vec3(0.61f, 0.48f, 0.29f);
                    const glm::vec3 lightBand = style == SurfaceStyle::Jupiter
                        ? glm::vec3(0.91f, 0.72f, 0.49f)
                        : glm::vec3(0.96f, 0.86f, 0.65f);
                    color = glm::mix(
                        darkBand,
                        lightBand,
                        glm::smoothstep(-0.22f, 0.30f, bands)
                    );
                    color *= 0.87f + fineNoise * 0.22f;
                    if (style == SurfaceStyle::Jupiter) {
                        const float wrappedLongitude = std::remainder(
                            longitude + 0.85f,
                            2.0f * PI
                        );
                        const float spotDistance = glm::length(glm::vec2(
                            wrappedLongitude * 0.58f,
                            latitude + 0.20f
                        ));
                        const float redSpot = 1.0f - glm::smoothstep(
                            0.085f,
                            0.17f,
                            spotDistance
                        );
                        color = glm::mix(
                            color,
                            glm::vec3(0.72f, 0.25f, 0.13f),
                            redSpot * 0.88f
                        );
                    }
                    break;
                }
                case SurfaceStyle::Uranus: {
                    const float atmosphere = surfaceNoise(
                        direction * 5.0f + glm::vec3(17.0f, 3.0f, 28.0f)
                    );
                    const float bands = std::sin(
                        latitude * 12.0f + (atmosphere - 0.5f) * 2.0f
                    );
                    color = glm::mix(
                        glm::vec3(0.22f, 0.54f, 0.61f),
                        glm::vec3(0.60f, 0.84f, 0.84f),
                        glm::smoothstep(-0.65f, 0.58f, bands)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(0.80f, 0.92f, 0.90f),
                        glm::smoothstep(0.79f, 0.96f, std::abs(latitude)) * 0.35f
                    );
                    break;
                }
                case SurfaceStyle::Neptune: {
                    const float atmosphere = surfaceNoise(
                        direction * 8.0f + glm::vec3(25.0f, 6.0f, 2.0f)
                    );
                    const float bands = std::sin(
                        latitude * 19.0f + std::sin(longitude * 2.0f) * 0.45f +
                        (atmosphere - 0.5f) * 2.1f
                    );
                    color = glm::mix(
                        glm::vec3(0.035f, 0.17f, 0.52f),
                        glm::vec3(0.12f, 0.48f, 0.91f),
                        glm::smoothstep(-0.65f, 0.70f, bands)
                    );
                    const float wrappedLongitude = std::remainder(
                        longitude - 1.0f,
                        2.0f * PI
                    );
                    const float stormDistance = glm::length(glm::vec2(
                        wrappedLongitude * 0.65f,
                        latitude + 0.18f
                    ));
                    const float storm = 1.0f - glm::smoothstep(
                        0.075f,
                        0.16f,
                        stormDistance
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(0.025f, 0.075f, 0.25f),
                        storm * 0.78f
                    );
                    break;
                }
                case SurfaceStyle::Moon: {
                    const float craterField = surfaceNoise(
                        direction * 15.0f + glm::vec3(5.0f, 17.0f, 9.0f)
                    );
                    const float highlands = surfaceNoise(
                        direction * 4.0f + glm::vec3(23.0f, 8.0f, 2.0f)
                    );
                    color = glm::mix(
                        glm::vec3(0.25f, 0.25f, 0.26f),
                        glm::vec3(0.75f, 0.72f, 0.67f),
                        glm::smoothstep(0.25f, 0.77f, highlands)
                    );
                    color *= 0.87f + glm::smoothstep(0.38f, 0.72f, craterField) * 0.24f;
                    break;
                }
                case SurfaceStyle::Pluto: {
                    const float terrain = surfaceNoise(
                        direction * 5.0f + glm::vec3(9.0f, 27.0f, 3.0f)
                    );
                    const float nitrogenIce = surfaceNoise(
                        direction * 11.0f + glm::vec3(1.0f, 18.0f, 24.0f)
                    );
                    color = glm::mix(
                        glm::vec3(0.25f, 0.20f, 0.17f),
                        glm::vec3(0.72f, 0.56f, 0.42f),
                        glm::smoothstep(0.25f, 0.76f, terrain)
                    );
                    color = glm::mix(
                        color,
                        glm::vec3(0.91f, 0.83f, 0.70f),
                        glm::smoothstep(0.62f, 0.79f, nitrogenIce) * 0.72f
                    );
                    break;
                }
            }

            if (style != SurfaceStyle::Sun) {
                color = applyPlanetLighting(color, direction, lightDirection);
                color *= 0.96f + grain * 0.08f;
            }
            return glm::vec4(color, baseColor.a);
        }

        void addSphere(
            PointCloud& cloud,
            const glm::vec3& center,
            float radius,
            std::size_t pointCount,
            const glm::vec4& color,
            const PointOrbitMotion& motion = {},
            SurfaceStyle style = SurfaceStyle::Moon,
            const glm::vec3& lightDirection = glm::vec3(0.0f, 0.0f, 1.0f)
        ) {
            const glm::vec3 spinAxis = glm::normalize(motion.spinAxis);
            for (std::size_t i = 0; i < pointCount; ++i) {
                const float fraction =
                    (static_cast<float>(i) + 0.5f) /
                    static_cast<float>(pointCount);
                const float y = 1.0f - 2.0f * fraction;
                const float ringRadius = std::sqrt(
                    std::max(0.0f, 1.0f - y * y)
                );
                const float angle = GOLDEN_ANGLE * static_cast<float>(i);
                const glm::vec3 direction(
                    std::cos(angle) * ringRadius,
                    y,
                    std::sin(angle) * ringRadius
                );
                const glm::vec4 pointColor = surfaceColor(
                    style,
                    color,
                    direction,
                    spinAxis,
                    lightDirection,
                    i
                );
                cloud.addPoint(
                    center + direction * radius,
                    pointColor,
                    1.0f,
                    {},
                    motion
                );
            }
        }

        void addOrbit(
            PointCloud& cloud,
            float radius,
            const glm::vec4& color,
            std::size_t pointCount = 5760
        ) {
            for (std::size_t i = 0; i < pointCount; ++i) {
                const float angle =
                    2.0f * PI * static_cast<float>(i) /
                    static_cast<float>(pointCount);
                cloud.addPoint(orbitPosition(radius, angle), color);
            }
        }

        void addSaturnRings(
            PointCloud& cloud,
            const glm::vec3& center,
            const PointOrbitMotion& motion
        ) {
            constexpr std::size_t POINTS_PER_RING = 4800;
            const std::array<float, 4> radii = {
                0.069f, 0.076f, 0.083f, 0.090f
            };
            const std::array<glm::vec4, 4> ringColors = {{
                {0.60f, 0.48f, 0.32f, 0.62f},
                {0.86f, 0.76f, 0.59f, 0.78f},
                {0.95f, 0.87f, 0.70f, 0.82f},
                {0.70f, 0.60f, 0.44f, 0.62f}
            }};
            const glm::vec3 ringNormal = glm::normalize(motion.spinAxis);
            const glm::vec3 reference = std::abs(ringNormal.y) > 0.9f
                ? glm::vec3(1.0f, 0.0f, 0.0f)
                : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::vec3 ringTangent = glm::normalize(
                glm::cross(reference, ringNormal)
            );
            const glm::vec3 ringBitangent = glm::normalize(
                glm::cross(ringNormal, ringTangent)
            );

            for (std::size_t ring = 0; ring < radii.size(); ++ring) {
                for (std::size_t i = 0; i < POINTS_PER_RING; ++i) {
                    const float angle = 2.0f * PI *
                        static_cast<float>(i) /
                        static_cast<float>(POINTS_PER_RING);
                    const float radius = radii[ring] +
                        static_cast<float>(i % 7) * 0.00055f;
                    cloud.addPoint(
                        center + ringTangent * (std::cos(angle) * radius) +
                            ringBitangent * (std::sin(angle) * radius) +
                            ringNormal * (static_cast<float>(ring) * 0.002f),
                        ringColors[ring],
                        1.0f,
                        {},
                        motion
                    );
                }
            }
        }

        void addGravityArrows(PointCloud& cloud) {
            const glm::vec4 fieldColor(0.12f, 0.66f, 0.82f, 0.76f);
            const glm::vec3 sunCenter(0.0f, ORBIT_PLANE_HEIGHT, 0.0f);

            for (const Planet& planet : PLANETS) {
                const glm::vec3 planetCenter = planetPosition(planet, 0.0f);
                const PointOrbitMotion motion = orbitAroundSun(planet.orbitalSpeed);
                const glm::vec3 direction = glm::normalize(sunCenter - planetCenter);
                for (int step = 0; step <= 112; ++step) {
                    const float t = static_cast<float>(step) / 112.0f;
                    cloud.addPoint(
                        glm::mix(planetCenter, sunCenter, t),
                        fieldColor,
                        1.0f,
                        {},
                        motion
                    );
                }

                const glm::vec3 tip = sunCenter + direction * 0.16f;
                const glm::vec3 base = sunCenter + direction * 0.23f;
                const glm::vec3 perpendicular(-direction.y, direction.x, 0.0f);
                const glm::vec3 left = base + perpendicular * 0.045f;
                const glm::vec3 right = base - perpendicular * 0.045f;
                for (int step = 0; step <= 32; ++step) {
                    const float t = static_cast<float>(step) / 32.0f;
                    cloud.addPoint(glm::mix(tip, left, t), fieldColor, 1.0f, {}, motion);
                    cloud.addPoint(glm::mix(tip, right, t), fieldColor, 1.0f, {}, motion);
                }
            }
        }

        glm::vec3 moonOffset(float elapsedTime) {
            const float angle = MOON_ORBIT_SPEED * ORBIT_SPEED_SCALE * elapsedTime;
            return glm::vec3(
                std::cos(angle) * MOON_ORBIT_RADIUS,
                std::sin(angle) * MOON_ORBIT_RADIUS,
                0.006f
            );
        }

        std::string lowercase(std::string_view text) {
            std::string result;
            result.reserve(text.size());
            for (const unsigned char character : text) {
                result.push_back(static_cast<char>(std::tolower(character)));
            }
            return result;
        }

    }

    bool SolarSystemLesson::applyTranscript(std::string_view transcript) {
        const std::string text = lowercase(transcript);
        bool changed = false;

        if (
            text.find("solar system") != std::string::npos &&
            text.find("sun") != std::string::npos &&
            text.find("orbit") != std::string::npos &&
            !solarSystemVisible_
        ) {
            solarSystemVisible_ = true;
            changed = true;
        }

        if (
            solarSystemVisible_ &&
            text.find("gravity") != std::string::npos &&
            (text.find("orbit") != std::string::npos ||
             text.find("planet") != std::string::npos) &&
            !gravityExplanationVisible_
        ) {
            gravityExplanationVisible_ = true;
            changed = true;
        }

        return changed;
    }

    void SolarSystemLesson::clear() {
        solarSystemVisible_ = false;
        gravityExplanationVisible_ = false;
    }

    bool SolarSystemLesson::hasSolarSystem() const {
        return solarSystemVisible_;
    }

    bool SolarSystemLesson::hasGravityExplanation() const {
        return gravityExplanationVisible_;
    }

    PointCloud SolarSystemLesson::buildPointCloud() const {
        PointCloud cloud("Solar System Presentation");
        if (!solarSystemVisible_) {
            return cloud;
        }

        const glm::vec4 orbitColor(0.28f, 0.40f, 0.50f, 0.38f);
        const glm::vec3 sunCenter(0.0f, ORBIT_PLANE_HEIGHT, 0.0f);
        addSphere(
            cloud,
            sunCenter,
            SUN_RADIUS,
            38400,
            glm::vec4(1.0f, 0.84f, 0.52f, 1.0f),
            spinningBodyMotion(sunCenter, 0.0f, SUN_SPIN_SPEED, 7.25f),
            SurfaceStyle::Sun
        );

        for (const Planet& planet : PLANETS) {
            addOrbit(cloud, planet.orbitRadius, orbitColor);
            const glm::vec3 center = planetPosition(planet, 0.0f);
            const PointOrbitMotion motion = spinningBodyMotion(
                center,
                planet.orbitalSpeed,
                planet.spinSpeed,
                planet.axialTilt
            );
            addSphere(
                cloud,
                center,
                planet.radius,
                planet.pointCount * PLANET_SURFACE_POINT_MULTIPLIER,
                planet.color,
                motion,
                planet.surfaceStyle,
                glm::normalize(-center)
            );
            if (std::string(planet.name) == "Saturn") {
                addSaturnRings(cloud, center, motion);
            }
        }

        // One representative moon follows Earth's solar orbit and loops around Earth.
        const Planet& earthPlanet = PLANETS[2];
        const glm::vec3 earth = planetPosition(earthPlanet, 0.0f);
        PointOrbitMotion moonMotion = orbitAroundSun(earthPlanet.orbitalSpeed);
        moonMotion.secondaryCenter = earth;
        moonMotion.secondaryAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        moonMotion.secondaryAngularSpeed =
            MOON_ORBIT_SPEED * ORBIT_SPEED_SCALE;
        addSphere(
            cloud,
            earth + moonOffset(0.0f),
            0.014f,
            512,
            MOON_COLOR,
            moonMotion,
            SurfaceStyle::Moon,
            glm::normalize(-earth)
        );

        const glm::vec3 pluto = orbitPosition(PLUTO_ORBIT, 2.25f);
        addSphere(
            cloud,
            pluto,
            0.018f,
            576,
            PLUTO_COLOR,
            orbitAroundSun(PLUTO_SPEED),
            SurfaceStyle::Pluto,
            glm::normalize(-pluto)
        );

        const std::vector<PointCloudPoint> solarSystemPoints = cloud.getPoints();
        if (gravityExplanationVisible_) {
            addGravityArrows(cloud);
        }

        const std::size_t paddingCount =
            LESSON_POINT_COUNT > cloud.getPointCount()
                ? LESSON_POINT_COUNT - cloud.getPointCount()
                : 0;
        for (std::size_t paddingIndex = 0; paddingIndex < paddingCount; ++paddingIndex) {
            const std::size_t sourceIndex =
                (paddingIndex * solarSystemPoints.size()) / paddingCount;
            const PointCloudPoint& source = solarSystemPoints[sourceIndex];
            const float angle = static_cast<float>(paddingIndex) * GOLDEN_ANGLE;
            const float jitter = 0.0012f *
                (0.5f + 0.5f * std::sin(static_cast<float>(paddingIndex)));
            const glm::vec3 offset(
                std::cos(angle) * jitter,
                std::sin(angle) * jitter,
                std::sin(angle * 0.73f) * jitter
            );
            glm::vec4 dimmedColor = source.color;
            dimmedColor.a *= 0.38f;
            cloud.addPoint(
                source.position + offset,
                dimmedColor,
                source.size,
                {},
                source.orbitMotion
            );
        }

        PointCloud framedCloud("Solar System Presentation");
        for (const PointCloudPoint& point : cloud.getPoints()) {
            framedCloud.addPoint(
                framePosition(point.position),
                point.color,
                point.size,
                {},
                frameOrbitMotion(point.orbitMotion)
            );
        }
        return framedCloud;
    }

    std::array<SolarSystemLessonLabel, 11> SolarSystemLesson::getLabels(
        float elapsedTime
    ) const {
        elapsedTime = std::max(0.0f, elapsedTime);
        std::array<SolarSystemLessonLabel, 11> labels{};
        labels[0] = {
            "Sun",
            framePosition(glm::vec3(0.0f, ORBIT_PLANE_HEIGHT, 0.0f)),
            glm::vec4(1.0f, 0.84f, 0.52f, 1.0f)
        };

        for (std::size_t i = 0; i < PLANETS.size(); ++i) {
            labels[i + 1] = {
                PLANETS[i].name,
                framePosition(planetPosition(PLANETS[i], elapsedTime)),
                PLANETS[i].color
            };
        }

        const glm::vec3 earth = planetPosition(PLANETS[2], elapsedTime);
        labels[9] = {
            "Moon",
            framePosition(earth + moonOffset(elapsedTime)),
            MOON_COLOR
        };
        labels[10] = {
            "Pluto - dwarf planet",
            framePosition(orbitPosition(
                PLUTO_ORBIT,
                2.25f + PLUTO_SPEED * ORBIT_SPEED_SCALE * elapsedTime
            )),
            PLUTO_COLOR
        };
        return labels;
    }

}
