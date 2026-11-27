#include "anatomy_lesson.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace chionia {

    namespace {

        constexpr float PI = 3.14159265359f;
        constexpr float GOLDEN_ANGLE = 2.39996322973f;
        constexpr float FLOW_AXIS_Z = 0.18f;

        const glm::vec4 DEOXYGENATED(0.06f, 0.42f, 0.92f, 0.88f);
        const glm::vec4 OXYGENATED(0.96f, 0.19f, 0.22f, 0.90f);
        const glm::vec4 MYOCARDIUM(0.84f, 0.26f, 0.39f, 0.78f);
        const glm::vec4 VALVE_COLOR(1.0f, 0.74f, 0.31f, 0.94f);
        const glm::vec4 CONDUCTION_COLOR(0.98f, 0.91f, 0.48f, 0.96f);

        std::string lowercase(std::string_view text) {
            std::string result;
            result.reserve(text.size());
            for (const unsigned char character : text) {
                if (character >= 'A' && character <= 'Z') {
                    result.push_back(static_cast<char>(character - 'A' + 'a'));
                }
                else {
                    result.push_back(static_cast<char>(character));
                }
            }
            return result;
        }

        glm::vec3 sphereDirection(std::size_t index, std::size_t count) {
            const float fraction =
                (static_cast<float>(index) + 0.5f) /
                static_cast<float>(count);
            const float y = 1.0f - 2.0f * fraction;
            const float ringRadius = std::sqrt(
                std::max(0.0f, 1.0f - y * y)
            );
            const float angle = GOLDEN_ANGLE * static_cast<float>(index);
            return glm::vec3(
                std::cos(angle) * ringRadius,
                y,
                std::sin(angle) * ringRadius
            );
        }

        glm::vec3 bezierPoint(
            const glm::vec3& start,
            const glm::vec3& controlA,
            const glm::vec3& controlB,
            const glm::vec3& end,
            float t
        ) {
            const float inverseT = 1.0f - t;
            return inverseT * inverseT * inverseT * start +
                3.0f * inverseT * inverseT * t * controlA +
                3.0f * inverseT * t * t * controlB +
                t * t * t * end;
        }

        glm::vec3 bezierTangent(
            const glm::vec3& start,
            const glm::vec3& controlA,
            const glm::vec3& controlB,
            const glm::vec3& end,
            float t
        ) {
            const float inverseT = 1.0f - t;
            return 3.0f * inverseT * inverseT * (controlA - start) +
                6.0f * inverseT * t * (controlB - controlA) +
                3.0f * t * t * (end - controlB);
        }

        void addBezierTube(
            PointCloud& cloud,
            const glm::vec3& start,
            const glm::vec3& controlA,
            const glm::vec3& controlB,
            const glm::vec3& end,
            float radius,
            const glm::vec4& color,
            int segments = 72,
            int sides = 10
        ) {
            for (int segment = 0; segment <= segments; ++segment) {
                const float t = static_cast<float>(segment) /
                    static_cast<float>(segments);
                const glm::vec3 center = bezierPoint(
                    start,
                    controlA,
                    controlB,
                    end,
                    t
                );
                const glm::vec3 tangent = glm::normalize(bezierTangent(
                    start,
                    controlA,
                    controlB,
                    end,
                    t
                ));
                const glm::vec3 reference =
                    std::abs(glm::dot(tangent, glm::vec3(0.0f, 0.0f, 1.0f))) > 0.92f
                        ? glm::vec3(0.0f, 1.0f, 0.0f)
                        : glm::vec3(0.0f, 0.0f, 1.0f);
                const glm::vec3 normal = glm::normalize(
                    glm::cross(tangent, reference)
                );
                const glm::vec3 binormal = glm::normalize(
                    glm::cross(tangent, normal)
                );

                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * PI *
                        static_cast<float>(side) /
                        static_cast<float>(sides);
                    const glm::vec3 ringOffset = radius * (
                        std::cos(angle) * normal +
                        std::sin(angle) * binormal
                    );
                    cloud.addPoint(center + ringOffset, color);
                }
            }

            // A fine central highlight keeps each tube legible at lecture scale.
            for (int segment = 0; segment <= segments; ++segment) {
                const float t = static_cast<float>(segment) /
                    static_cast<float>(segments);
                cloud.addPoint(
                    bezierPoint(start, controlA, controlB, end, t),
                    glm::vec4(color.r, color.g, color.b, color.a * 0.52f),
                    0.85f
                );
            }
        }

        void addStraightTube(
            PointCloud& cloud,
            const glm::vec3& start,
            const glm::vec3& end,
            float radius,
            const glm::vec4& color,
            int sides = 8
        ) {
            const glm::vec3 delta = end - start;
            addBezierTube(
                cloud,
                start,
                start + delta / 3.0f,
                start + delta * (2.0f / 3.0f),
                end,
                radius,
                color,
                28,
                sides
            );
        }

        void addArrowHead(
            PointCloud& cloud,
            const glm::vec3& tip,
            const glm::vec3& direction,
            float size,
            const glm::vec4& color
        ) {
            const glm::vec3 forward = glm::normalize(direction);
            const glm::vec3 side(-forward.y, forward.x, 0.0f);
            const glm::vec3 base = tip - forward * size;
            const glm::vec3 left = base + side * (size * 0.48f);
            const glm::vec3 right = base - side * (size * 0.48f);
            addStraightTube(cloud, tip, left, 0.006f, color, 5);
            addStraightTube(cloud, tip, right, 0.006f, color, 5);
        }

        void addEllipsoidShell(
            PointCloud& cloud,
            const glm::vec3& center,
            const glm::vec3& radii,
            std::size_t pointCount,
            const glm::vec4& startColor,
            const glm::vec4& endColor
        ) {
            for (std::size_t index = 0; index < pointCount; ++index) {
                const glm::vec3 direction = sphereDirection(index, pointCount);
                const float variation = 0.5f + 0.5f * std::sin(
                    static_cast<float>(index) * 0.057f
                );
                cloud.addPoint(
                    center + direction * radii,
                    glm::mix(startColor, endColor, variation)
                );
            }
        }

        void addBloodChamber(
            PointCloud& cloud,
            const glm::vec3& center,
            const glm::vec3& radii,
            const glm::vec4& bloodColor,
            std::size_t shellCount,
            std::size_t lumenCount
        ) {
            const glm::vec4 thinWall(
                MYOCARDIUM.r,
                MYOCARDIUM.g,
                MYOCARDIUM.b,
                MYOCARDIUM.a * 0.72f
            );
            addEllipsoidShell(
                cloud,
                center,
                radii,
                shellCount,
                thinWall,
                glm::vec4(0.98f, 0.51f, 0.54f, 0.78f)
            );

            for (std::size_t index = 0; index < lumenCount; ++index) {
                const glm::vec3 direction = sphereDirection(index, lumenCount);
                const float radius = 0.12f + 0.72f * std::cbrt(
                    std::fmod(
                        static_cast<float>(index) * 0.754877666f + 0.31f,
                        1.0f
                    )
                );
                const float tint = 0.78f + 0.22f * std::sin(
                    static_cast<float>(index) * 0.19f
                );
                cloud.addPoint(
                    center + direction * (radii * radius),
                    glm::vec4(
                        bloodColor.r * tint,
                        bloodColor.g * tint,
                        bloodColor.b * tint,
                        bloodColor.a * 0.34f
                    ),
                    0.9f
                );
            }
        }

        void addThickLeftVentricularWall(
            PointCloud& cloud,
            const glm::vec3& center,
            const glm::vec3& outerRadii
        ) {
            constexpr std::size_t WALL_POINTS = 2400;
            const glm::vec4 muscleColor(0.95f, 0.34f, 0.43f, 0.46f);
            for (std::size_t index = 0; index < WALL_POINTS; ++index) {
                const glm::vec3 direction = sphereDirection(index, WALL_POINTS);
                const float radialFraction = 0.74f + 0.23f * std::fmod(
                    static_cast<float>(index) * 0.61803398875f,
                    1.0f
                );
                cloud.addPoint(
                    center + direction * (outerRadii * radialFraction),
                    muscleColor,
                    0.88f
                );
            }
        }

        void addValve(
            PointCloud& cloud,
            const glm::vec3& center,
            float radius,
            int leafletCount = 3
        ) {
            constexpr int RING_POINTS = 96;
            for (int index = 0; index < RING_POINTS; ++index) {
                const float angle = 2.0f * PI *
                    static_cast<float>(index) /
                    static_cast<float>(RING_POINTS);
                cloud.addPoint(
                    center + glm::vec3(
                        radius * std::cos(angle),
                        radius * 0.68f * std::sin(angle),
                        0.0f
                    ),
                    VALVE_COLOR,
                    0.92f
                );
            }

            for (int leaflet = 0; leaflet < leafletCount; ++leaflet) {
                const float angle = 2.0f * PI *
                    static_cast<float>(leaflet) /
                    static_cast<float>(leafletCount);
                const glm::vec3 tip = center + glm::vec3(
                    std::cos(angle) * radius * 0.75f,
                    std::sin(angle) * radius * 0.45f,
                    0.004f
                );
                const glm::vec3 hinge = center + glm::vec3(
                    std::cos(angle + PI) * radius * 0.38f,
                    std::sin(angle + PI) * radius * 0.22f,
                    0.004f
                );
                addStraightTube(cloud, tip, hinge, 0.004f, VALVE_COLOR, 4);
            }
        }

        void addLungSymbol(
            PointCloud& cloud,
            const glm::vec3& center,
            float width,
            float height
        ) {
            constexpr std::size_t OUTLINE_POINTS = 1400;
            addEllipsoidShell(
                cloud,
                center,
                glm::vec3(width, height, 0.09f),
                OUTLINE_POINTS,
                glm::vec4(0.08f, 0.34f, 0.54f, 0.42f),
                glm::vec4(0.12f, 0.61f, 0.72f, 0.66f)
            );

            // Sparse alveolar-capillary marks keep the lungs a supporting
            // exchange site instead of another large opaque surface.
            constexpr int BRANCHES = 10;
            constexpr int SAMPLES = 28;
            for (int branch = 0; branch < BRANCHES; ++branch) {
                const float phase = 2.0f * PI *
                    static_cast<float>(branch) /
                    static_cast<float>(BRANCHES);
                const float startX = std::cos(phase) * width * 0.12f;
                const float startY = -height * 0.32f;
                const glm::vec4 color = branch % 2 == 0
                    ? glm::vec4(0.12f, 0.52f, 0.92f, 0.82f)
                    : glm::vec4(0.92f, 0.24f, 0.30f, 0.86f);
                for (int sample = 0; sample < SAMPLES; ++sample) {
                    const float t = static_cast<float>(sample) /
                        static_cast<float>(SAMPLES - 1);
                    const float x = startX +
                        std::cos(phase + t * 2.2f) * width * 0.58f;
                    const float y = startY + t * height * 0.82f;
                    cloud.addPoint(
                        center + glm::vec3(x, y, 0.10f),
                        glm::mix(color, OXYGENATED, t * 0.58f),
                        0.78f
                    );
                }
            }
        }

        void addHeartSilhouette(PointCloud& cloud) {
            const glm::vec4 edgeColor(0.66f, 0.16f, 0.28f, 0.72f);
            addBezierTube(
                cloud,
                glm::vec3(-0.39f, 0.54f, -0.02f),
                glm::vec3(-0.74f, 0.42f, -0.02f),
                glm::vec3(-0.75f, -0.18f, -0.02f),
                glm::vec3(-0.42f, -0.57f, -0.02f),
                0.012f,
                edgeColor,
                100,
                7
            );
            addBezierTube(
                cloud,
                glm::vec3(-0.42f, -0.57f, -0.02f),
                glm::vec3(-0.28f, -0.76f, -0.02f),
                glm::vec3(-0.10f, -0.88f, -0.02f),
                glm::vec3(-0.02f, -0.91f, -0.02f),
                0.012f,
                edgeColor,
                60,
                7
            );
            addBezierTube(
                cloud,
                glm::vec3(0.39f, 0.54f, -0.02f),
                glm::vec3(0.73f, 0.36f, -0.02f),
                glm::vec3(0.65f, -0.16f, -0.02f),
                glm::vec3(0.39f, -0.56f, -0.02f),
                0.012f,
                edgeColor,
                100,
                7
            );
            addBezierTube(
                cloud,
                glm::vec3(0.39f, -0.56f, -0.02f),
                glm::vec3(0.23f, -0.76f, -0.02f),
                glm::vec3(0.05f, -0.88f, -0.02f),
                glm::vec3(-0.02f, -0.91f, -0.02f),
                0.012f,
                edgeColor,
                60,
                7
            );
        }

        void addConductionPath(PointCloud& cloud) {
            const glm::vec3 sinoatrialNode(-0.36f, 0.45f, 0.245f);
            const glm::vec3 atrioventricularNode(-0.01f, 0.08f, 0.25f);
            addBezierTube(
                cloud,
                sinoatrialNode,
                glm::vec3(-0.18f, 0.43f, 0.25f),
                glm::vec3(0.16f, 0.28f, 0.25f),
                atrioventricularNode,
                0.006f,
                CONDUCTION_COLOR,
                34,
                5
            );
            addStraightTube(
                cloud,
                atrioventricularNode,
                glm::vec3(-0.01f, -0.32f, 0.25f),
                0.007f,
                CONDUCTION_COLOR,
                5
            );
            addBezierTube(
                cloud,
                glm::vec3(-0.01f, -0.24f, 0.25f),
                glm::vec3(-0.12f, -0.35f, 0.25f),
                glm::vec3(-0.25f, -0.49f, 0.25f),
                glm::vec3(-0.31f, -0.66f, 0.25f),
                0.005f,
                CONDUCTION_COLOR,
                28,
                5
            );
            addBezierTube(
                cloud,
                glm::vec3(-0.01f, -0.24f, 0.25f),
                glm::vec3(0.12f, -0.35f, 0.25f),
                glm::vec3(0.25f, -0.49f, 0.25f),
                glm::vec3(0.31f, -0.66f, 0.25f),
                0.005f,
                CONDUCTION_COLOR,
                28,
                5
            );
            addEllipsoidShell(
                cloud,
                sinoatrialNode,
                glm::vec3(0.026f),
                220,
                glm::vec4(1.0f, 0.94f, 0.55f, 0.94f),
                glm::vec4(1.0f, 1.0f, 0.77f, 1.0f)
            );
            addEllipsoidShell(
                cloud,
                atrioventricularNode,
                glm::vec3(0.020f),
                180,
                glm::vec4(1.0f, 0.84f, 0.44f, 0.92f),
                glm::vec4(1.0f, 0.96f, 0.63f, 1.0f)
            );
        }

        void addSystemicArrow(
            PointCloud& cloud,
            const glm::vec3& tip,
            const glm::vec3& direction,
            const glm::vec4& color
        ) {
            addArrowHead(cloud, tip, direction, 0.072f, color);
        }

    }

    bool AnatomyLesson::recognizesTranscript(
        std::string_view transcript
    ) const {
        const std::string text = lowercase(transcript);
        // The sample lecture describes a cardiac cycle without using the word
        // "heart" in its opening sentence. Treat that specific term as a
        // direct anatomy signal so the supplied example is recognized.
        const bool mentionsHeart =
            text.find("heart") != std::string::npos ||
            text.find("cardiac cycle") != std::string::npos;
        const bool mentionsCirculation =
            text.find("blood") != std::string::npos ||
            text.find("cardiac cycle") != std::string::npos ||
            text.find("circulation") != std::string::npos;
        const bool mentionsAnatomy =
            text.find("chamber") != std::string::npos ||
            text.find("atrium") != std::string::npos ||
            text.find("ventricle") != std::string::npos ||
            text.find("valve") != std::string::npos;
        return mentionsHeart && (mentionsCirculation || mentionsAnatomy);
    }

    bool AnatomyLesson::applyTranscript(std::string_view transcript) {
        if (!recognizesTranscript(transcript) || heartCycleVisible_) {
            return false;
        }
        heartCycleVisible_ = true;
        return true;
    }

    void AnatomyLesson::clear() {
        heartCycleVisible_ = false;
    }

    bool AnatomyLesson::hasHeartCycle() const {
        return heartCycleVisible_;
    }

    PointCloud AnatomyLesson::buildPointCloud() const {
        PointCloud cloud("Heart and Cardiac Cycle");
        if (!heartCycleVisible_) {
            return cloud;
        }

        addHeartSilhouette(cloud);

        // An anterior cutaway: the patient's right is on the viewer's left.
        // The lower-right chamber has the more substantial muscular wall of
        // the left ventricle, which must generate systemic pressure.
        addBloodChamber(
            cloud,
            glm::vec3(-0.28f, 0.31f, 0.045f),
            glm::vec3(0.205f, 0.19f, 0.15f),
            DEOXYGENATED,
            3900,
            2100
        );
        addBloodChamber(
            cloud,
            glm::vec3(-0.27f, -0.20f, 0.055f),
            glm::vec3(0.235f, 0.32f, 0.17f),
            DEOXYGENATED,
            5100,
            2500
        );
        addBloodChamber(
            cloud,
            glm::vec3(0.27f, 0.31f, 0.035f),
            glm::vec3(0.19f, 0.18f, 0.15f),
            OXYGENATED,
            3700,
            1900
        );
        addBloodChamber(
            cloud,
            glm::vec3(0.27f, -0.20f, 0.035f),
            glm::vec3(0.275f, 0.365f, 0.235f),
            OXYGENATED,
            6900,
            3100
        );
        addThickLeftVentricularWall(
            cloud,
            glm::vec3(0.27f, -0.20f, 0.035f),
            glm::vec3(0.275f, 0.365f, 0.235f)
        );

        // Septa divide the four chambers. A single center line is used for
        // each wall to keep this a readable cutaway rather than a mesh cage.
        addStraightTube(
            cloud,
            glm::vec3(0.0f, 0.44f, 0.20f),
            glm::vec3(0.0f, 0.14f, 0.20f),
            0.010f,
            glm::vec4(0.83f, 0.40f, 0.48f, 0.78f),
            6
        );
        addStraightTube(
            cloud,
            glm::vec3(0.0f, 0.08f, 0.20f),
            glm::vec3(0.0f, -0.65f, 0.20f),
            0.018f,
            glm::vec4(0.91f, 0.40f, 0.47f, 0.92f),
            8
        );

        // Atrioventricular and semilunar valves enforce one-way flow.
        addValve(cloud, glm::vec3(-0.27f, 0.035f, 0.245f), 0.075f, 3);
        addValve(cloud, glm::vec3(0.27f, 0.035f, 0.245f), 0.075f, 2);
        addValve(cloud, glm::vec3(-0.14f, -0.10f, 0.25f), 0.060f, 3);
        addValve(cloud, glm::vec3(0.12f, -0.07f, 0.25f), 0.060f, 3);

        // The venae cavae return lower-oxygen blood to the right atrium.
        addBezierTube(
            cloud,
            glm::vec3(-0.28f, 0.34f, 0.015f),
            glm::vec3(-0.39f, 0.57f, 0.015f),
            glm::vec3(-0.43f, 0.91f, 0.015f),
            glm::vec3(-0.25f, 1.08f, 0.015f),
            0.068f,
            DEOXYGENATED
        );
        addBezierTube(
            cloud,
            glm::vec3(-0.24f, -0.49f, 0.015f),
            glm::vec3(-0.29f, -0.67f, 0.015f),
            glm::vec3(-0.30f, -0.89f, 0.015f),
            glm::vec3(-0.25f, -1.06f, 0.015f),
            0.070f,
            DEOXYGENATED
        );

        // Pulmonary trunk and arteries carry lower-oxygen blood toward lungs.
        addBezierTube(
            cloud,
            glm::vec3(-0.13f, -0.06f, 0.14f),
            glm::vec3(-0.07f, 0.20f, 0.17f),
            glm::vec3(-0.06f, 0.50f, 0.18f),
            glm::vec3(-0.12f, 0.69f, 0.18f),
            0.065f,
            DEOXYGENATED
        );
        addBezierTube(
            cloud,
            glm::vec3(-0.12f, 0.65f, 0.18f),
            glm::vec3(-0.31f, 0.82f, 0.16f),
            glm::vec3(-0.56f, 0.91f, 0.12f),
            glm::vec3(-0.70f, 0.84f, 0.09f),
            0.041f,
            DEOXYGENATED
        );
        addBezierTube(
            cloud,
            glm::vec3(-0.12f, 0.65f, 0.18f),
            glm::vec3(0.08f, 0.83f, 0.16f),
            glm::vec3(0.45f, 0.90f, 0.12f),
            glm::vec3(0.70f, 0.84f, 0.09f),
            0.041f,
            DEOXYGENATED
        );

        // Paired lungs show the capillary site where gas exchange changes
        // blood oxygenation before the pulmonary veins return it to the heart.
        addLungSymbol(cloud, glm::vec3(-0.76f, 0.66f, -0.015f), 0.14f, 0.23f);
        addLungSymbol(cloud, glm::vec3(0.76f, 0.66f, -0.015f), 0.14f, 0.23f);
        addBezierTube(
            cloud,
            glm::vec3(-0.70f, 0.56f, 0.07f),
            glm::vec3(-0.49f, 0.52f, 0.13f),
            glm::vec3(-0.32f, 0.46f, 0.18f),
            glm::vec3(-0.13f, 0.37f, 0.20f),
            0.034f,
            OXYGENATED
        );
        addBezierTube(
            cloud,
            glm::vec3(0.70f, 0.56f, 0.07f),
            glm::vec3(0.50f, 0.53f, 0.13f),
            glm::vec3(0.38f, 0.46f, 0.18f),
            glm::vec3(0.13f, 0.37f, 0.20f),
            0.034f,
            OXYGENATED
        );

        // The aorta carries oxygen-rich blood from the left ventricle to the
        // systemic circuit; only its proximal arch is drawn in this model.
        addBezierTube(
            cloud,
            glm::vec3(0.14f, -0.045f, 0.16f),
            glm::vec3(0.36f, 0.12f, 0.17f),
            glm::vec3(0.42f, 0.51f, 0.17f),
            glm::vec3(0.19f, 0.73f, 0.17f),
            0.075f,
            OXYGENATED
        );
        addBezierTube(
            cloud,
            glm::vec3(0.19f, 0.73f, 0.17f),
            glm::vec3(0.04f, 0.92f, 0.15f),
            glm::vec3(-0.12f, 0.99f, 0.12f),
            glm::vec3(-0.22f, 1.07f, 0.10f),
            0.064f,
            OXYGENATED
        );

        // Clear directional cues distinguish the systemic veins, pulmonary
        // arteries, pulmonary veins, and the aortic outflow.
        addSystemicArrow(
            cloud,
            glm::vec3(-0.33f, 0.48f, 0.16f),
            glm::vec3(0.05f, -1.0f, 0.0f),
            DEOXYGENATED
        );
        addSystemicArrow(
            cloud,
            glm::vec3(-0.25f, -0.36f, 0.16f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            DEOXYGENATED
        );
        addSystemicArrow(
            cloud,
            glm::vec3(-0.10f, 0.41f, 0.24f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            DEOXYGENATED
        );
        addSystemicArrow(
            cloud,
            glm::vec3(0.33f, 0.43f, 0.24f),
            glm::vec3(-1.0f, -0.24f, 0.0f),
            OXYGENATED
        );
        addSystemicArrow(
            cloud,
            glm::vec3(0.38f, 0.40f, 0.24f),
            glm::vec3(0.10f, 1.0f, 0.0f),
            OXYGENATED
        );
        addSystemicArrow(
            cloud,
            glm::vec3(-0.21f, 0.99f, 0.18f),
            glm::vec3(-0.2f, 1.0f, 0.0f),
            OXYGENATED
        );

        addConductionPath(cloud);

        const std::size_t remainingPoints =
            MORPH_PARTICLE_COUNT > cloud.getPointCount()
                ? MORPH_PARTICLE_COUNT - cloud.getPointCount()
                : 0;
        for (std::size_t index = 0; index < remainingPoints; ++index) {
            const glm::vec3 direction = sphereDirection(
                index,
                std::max<std::size_t>(remainingPoints, 1)
            );
            cloud.addPoint(
                glm::vec3(direction.x * 0.38f, direction.y * 0.52f, direction.z * 0.16f),
                glm::vec4(0.0f)
            );
        }

        return cloud;
    }

    std::array<LessonLabel, 14> AnatomyLesson::getLabels() const {
        return {{
            {"Right atrium", glm::vec3(-0.28f, 0.31f, 0.24f), {0.24f, 0.60f, 1.0f, 1.0f}},
            {"Right ventricle", glm::vec3(-0.27f, -0.20f, 0.25f), {0.24f, 0.60f, 1.0f, 1.0f}},
            {"Left atrium", glm::vec3(0.27f, 0.31f, 0.24f), {1.0f, 0.40f, 0.40f, 1.0f}},
            {"Left ventricle", glm::vec3(0.27f, -0.20f, 0.27f), {1.0f, 0.40f, 0.40f, 1.0f}},
            {"Venae cavae", glm::vec3(-0.46f, 0.84f, 0.11f), {0.24f, 0.60f, 1.0f, 1.0f}},
            {"Pulmonary arteries", glm::vec3(-0.46f, 0.91f, 0.17f), {0.24f, 0.60f, 1.0f, 1.0f}},
            {"Lung capillaries", glm::vec3(0.77f, 0.69f, 0.18f), {0.42f, 0.80f, 0.86f, 1.0f}},
            {"Pulmonary veins", glm::vec3(0.52f, 0.50f, 0.21f), {1.0f, 0.40f, 0.40f, 1.0f}},
            {"Aorta", glm::vec3(0.42f, 0.80f, 0.23f), {1.0f, 0.40f, 0.40f, 1.0f}},
            {"Tricuspid valve", glm::vec3(-0.47f, 0.035f, 0.27f), {1.0f, 0.78f, 0.36f, 1.0f}},
            {"Mitral valve", glm::vec3(0.47f, 0.035f, 0.27f), {1.0f, 0.78f, 0.36f, 1.0f}},
            {"Pulmonary valve", glm::vec3(-0.19f, -0.10f, 0.29f), {1.0f, 0.78f, 0.36f, 1.0f}},
            {"Aortic valve", glm::vec3(0.10f, -0.07f, 0.29f), {1.0f, 0.78f, 0.36f, 1.0f}},
            {"Conduction pathway", glm::vec3(-0.03f, -0.44f, 0.29f), {1.0f, 0.91f, 0.51f, 1.0f}}
        }};
    }

}
