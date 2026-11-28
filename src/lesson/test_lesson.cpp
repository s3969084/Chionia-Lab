#include "test_lesson.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace chionia {

    namespace {

        constexpr float PI = 3.14159265359f;
        constexpr float GOLDEN_ANGLE = 2.39996322973f;
        constexpr float DETAIL_DENSITY_SCALE = 1.41421356237f;

        const glm::vec4 CYAN(0.10f, 0.78f, 1.0f, 0.95f);
        const glm::vec4 BLUE(0.12f, 0.42f, 0.98f, 0.92f);
        const glm::vec4 RED(1.0f, 0.28f, 0.24f, 0.94f);
        const glm::vec4 GOLD(1.0f, 0.71f, 0.18f, 0.96f);
        const glm::vec4 GREEN(0.25f, 0.88f, 0.50f, 0.91f);
        const glm::vec4 PALE(0.68f, 0.86f, 0.98f, 0.82f);

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

        void addSegment(
            PointCloud& cloud,
            const glm::vec3& start,
            const glm::vec3& end,
            const glm::vec4& color,
            int samples = 72,
            float pointSize = 1.0f
        ) {
            samples *= 2;
            for (int index = 0; index <= samples; ++index) {
                const float t = static_cast<float>(index) /
                    static_cast<float>(samples);
                cloud.addPoint(glm::mix(start, end, t), color, pointSize);
            }
        }

        void addTube(
            PointCloud& cloud,
            const glm::vec3& start,
            const glm::vec3& end,
            float radius,
            const glm::vec4& color,
            int samples = 64,
            int sides = 7
        ) {
            samples *= 2;
            const glm::vec3 tangent = glm::normalize(end - start);
            const glm::vec3 reference =
                std::abs(glm::dot(tangent, glm::vec3(0.0f, 0.0f, 1.0f))) > 0.9f
                    ? glm::vec3(0.0f, 1.0f, 0.0f)
                    : glm::vec3(0.0f, 0.0f, 1.0f);
            const glm::vec3 normal = glm::normalize(glm::cross(tangent, reference));
            const glm::vec3 binormal = glm::normalize(
                glm::cross(tangent, normal)
            );
            for (int sample = 0; sample <= samples; ++sample) {
                const float t = static_cast<float>(sample) /
                    static_cast<float>(samples);
                const glm::vec3 center = glm::mix(start, end, t);
                for (int side = 0; side < sides; ++side) {
                    const float angle = 2.0f * PI * static_cast<float>(side) /
                        static_cast<float>(sides);
                    const glm::vec3 offset = radius * (
                        std::cos(angle) * normal + std::sin(angle) * binormal
                    );
                    cloud.addPoint(center + offset, color);
                }
            }
            addSegment(cloud, start, end, color, samples / 2, 0.9f);
        }

        void addPolyline(
            PointCloud& cloud,
            const std::vector<glm::vec3>& points,
            const glm::vec4& color,
            int samplesPerSegment = 30
        ) {
            for (std::size_t index = 1; index < points.size(); ++index) {
                addSegment(
                    cloud,
                    points[index - 1],
                    points[index],
                    color,
                    samplesPerSegment
                );
            }
        }

        void addArrow(
            PointCloud& cloud,
            const glm::vec3& start,
            const glm::vec3& end,
            const glm::vec4& color,
            float headSize = 0.09f
        ) {
            addTube(cloud, start, end, 0.008f, color, 48, 5);
            const glm::vec3 direction = glm::normalize(end - start);
            const glm::vec3 side(-direction.y, direction.x, 0.0f);
            const glm::vec3 base = end - direction * headSize;
            addSegment(
                cloud,
                end,
                base + side * (headSize * 0.46f),
                color,
                18
            );
            addSegment(
                cloud,
                end,
                base - side * (headSize * 0.46f),
                color,
                18
            );
        }

        void addBezierLine(
            PointCloud& cloud,
            const glm::vec3& start,
            const glm::vec3& controlA,
            const glm::vec3& controlB,
            const glm::vec3& end,
            const glm::vec4& color,
            int samples = 100
        ) {
            glm::vec3 previous = start;
            for (int index = 1; index <= samples; ++index) {
                const float t = static_cast<float>(index) /
                    static_cast<float>(samples);
                const float inverseT = 1.0f - t;
                const glm::vec3 current = inverseT * inverseT * inverseT * start +
                    3.0f * inverseT * inverseT * t * controlA +
                    3.0f * inverseT * t * t * controlB + t * t * t * end;
                addSegment(cloud, previous, current, color, 5);
                previous = current;
            }
        }

        glm::vec3 sphereDirection(std::size_t index, std::size_t count) {
            const float fraction =
                (static_cast<float>(index) + 0.5f) / static_cast<float>(count);
            const float y = 1.0f - 2.0f * fraction;
            const float ringRadius = std::sqrt(std::max(0.0f, 1.0f - y * y));
            const float angle = GOLDEN_ANGLE * static_cast<float>(index);
            return glm::vec3(
                std::cos(angle) * ringRadius,
                y,
                std::sin(angle) * ringRadius
            );
        }

        void addSphere(
            PointCloud& cloud,
            const glm::vec3& center,
            const glm::vec3& radii,
            const glm::vec4& color,
            std::size_t points = 1600
        ) {
            points *= 2;
            for (std::size_t index = 0; index < points; ++index) {
                const glm::vec3 direction = sphereDirection(index, points);
                const float variation = 0.88f + 0.12f *
                    (0.5f + 0.5f * std::sin(static_cast<float>(index) * 0.19f));
                cloud.addPoint(
                    center + direction * radii,
                    glm::vec4(glm::vec3(color) * variation, color.a)
                );
            }
        }

        void addSphere(
            PointCloud& cloud,
            const glm::vec3& center,
            float radius,
            const glm::vec4& color,
            std::size_t points = 1600
        ) {
            addSphere(cloud, center, glm::vec3(radius), color, points);
        }

        void addGridPatch(
            PointCloud& cloud,
            float x0,
            float y0,
            float x1,
            float y1,
            float z,
            const glm::vec4& color,
            int xSteps = 28,
            int ySteps = 18
        ) {
            xSteps = static_cast<int>(std::ceil(xSteps * DETAIL_DENSITY_SCALE));
            ySteps = static_cast<int>(std::ceil(ySteps * DETAIL_DENSITY_SCALE));
            for (int row = 0; row <= ySteps; ++row) {
                const float v = static_cast<float>(row) /
                    static_cast<float>(ySteps);
                for (int column = 0; column <= xSteps; ++column) {
                    const float u = static_cast<float>(column) /
                        static_cast<float>(xSteps);
                    cloud.addPoint(
                        glm::vec3(
                            glm::mix(x0, x1, u),
                            glm::mix(y0, y1, v),
                            z
                        ),
                        color
                    );
                }
            }
        }

        void addQuadSurface(
            PointCloud& cloud,
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            const glm::vec3& d,
            const glm::vec4& color,
            int steps = 28
        ) {
            steps = static_cast<int>(std::ceil(steps * DETAIL_DENSITY_SCALE));
            for (int row = 0; row <= steps; ++row) {
                const float v = static_cast<float>(row) /
                    static_cast<float>(steps);
                for (int column = 0; column <= steps; ++column) {
                    const float u = static_cast<float>(column) /
                        static_cast<float>(steps);
                    const glm::vec3 lower = glm::mix(a, b, u);
                    const glm::vec3 upper = glm::mix(d, c, u);
                    cloud.addPoint(glm::mix(lower, upper, v), color);
                }
            }
        }

        void addCircle(
            PointCloud& cloud,
            const glm::vec3& center,
            float radius,
            float z,
            const glm::vec4& color,
            int points = 220
        ) {
            points *= 2;
            for (int index = 0; index < points; ++index) {
                const float angle = 2.0f * PI * static_cast<float>(index) /
                    static_cast<float>(points);
                cloud.addPoint(
                    glm::vec3(
                        center.x + std::cos(angle) * radius,
                        center.y + std::sin(angle) * radius,
                        z
                    ),
                    color
                );
            }
        }

        void addCircleNode(
            PointCloud& cloud,
            const glm::vec3& center,
            float radius,
            const glm::vec4& color
        ) {
            addSphere(cloud, center, radius, color, 1200);
            addCircle(
                cloud,
                center,
                radius * 1.08f,
                center.z + radius * 0.24f,
                glm::vec4(glm::vec3(color), 0.70f),
                280
            );
        }

        void addFish(
            PointCloud& cloud,
            const glm::vec3& center,
            float length,
            float height,
            const glm::vec4& color
        ) {
            addSphere(
                cloud,
                center,
                glm::vec3(length * 0.39f, height * 0.34f, height * 0.20f),
                color,
                1400
            );
            const float z = center.z + 0.02f;
            const glm::vec3 tailBase(center.x - length * 0.30f, center.y, z);
            addPolyline(
                cloud,
                {tailBase,
                 glm::vec3(center.x - length * 0.51f, center.y + height * 0.34f, z),
                 glm::vec3(center.x - length * 0.48f, center.y, z),
                 glm::vec3(center.x - length * 0.51f, center.y - height * 0.34f, z),
                 tailBase},
                color,
                20
            );
            addPolyline(
                cloud,
                {glm::vec3(center.x - length * 0.08f, center.y + height * 0.22f, z + 0.005f),
                 glm::vec3(center.x + length * 0.08f, center.y + height * 0.45f, z + 0.005f),
                 glm::vec3(center.x + length * 0.16f, center.y + height * 0.12f, z + 0.005f)},
                PALE,
                14
            );
            addSphere(
                cloud,
                glm::vec3(center.x + length * 0.22f, center.y + height * 0.08f, z + 0.02f),
                0.012f,
                PALE,
                80
            );
        }

        void addClosedSquare(
            PointCloud& cloud,
            const std::array<glm::vec3, 4>& corners,
            const glm::vec4& color,
            int steps = 34
        ) {
            addQuadSurface(
                cloud,
                corners[0],
                corners[1],
                corners[2],
                corners[3],
                glm::vec4(color.r, color.g, color.b, color.a * 0.27f),
                steps
            );
            for (std::size_t index = 0; index < corners.size(); ++index) {
                addSegment(
                    cloud,
                    corners[index],
                    corners[(index + 1) % corners.size()],
                    color,
                    70
                );
            }
        }

        void addCharge(
            PointCloud& cloud,
            const glm::vec3& center,
            const glm::vec4& color
        ) {
            addSphere(cloud, center, 0.045f, color, 220);
        }

        template<std::size_t N>
        glm::vec3 routePosition(
            const std::array<glm::vec3, N>& route,
            float animationTime,
            float period
        ) {
            const float phase = std::fmod(
                std::max(animationTime, 0.0f),
                period
            ) / period * static_cast<float>(N - 1);
            const std::size_t segment = std::min(
                static_cast<std::size_t>(phase),
                N - 2
            );
            return glm::mix(
                route[segment],
                route[segment + 1],
                phase - static_cast<float>(segment)
            );
        }

        void buildCircuit(PointCloud& cloud) {
            constexpr float left = -0.86f;
            constexpr float right = 0.86f;
            constexpr float top = 0.55f;
            constexpr float bottom = -0.55f;
            const glm::vec4 wire(0.12f, 0.65f, 0.96f, 0.88f);

            addTube(cloud, {-0.86f, 0.12f, 0.05f}, {left, top, 0.05f}, 0.014f, wire);
            addTube(cloud, {left, top, 0.05f}, {-0.34f, top, 0.05f}, 0.014f, wire);
            addTube(cloud, {0.34f, top, 0.05f}, {right, top, 0.05f}, 0.014f, wire);
            addTube(cloud, {right, top, 0.05f}, {right, 0.20f, 0.05f}, 0.014f, wire);
            addTube(cloud, {right, -0.20f, 0.05f}, {right, bottom, 0.05f}, 0.014f, wire);
            addTube(cloud, {right, bottom, 0.05f}, {left, bottom, 0.05f}, 0.014f, wire);
            addTube(cloud, {left, bottom, 0.05f}, {left, -0.12f, 0.05f}, 0.014f, wire);

            // One cell: long and short plates mark the battery terminals.
            addSegment(cloud, {-1.00f, 0.12f, 0.07f}, {-0.70f, 0.12f, 0.07f}, GOLD, 24, 1.5f);
            addSegment(cloud, {-0.94f, -0.12f, 0.07f}, {-0.78f, -0.12f, 0.07f}, GOLD, 18, 1.5f);
            addSegment(cloud, {-0.86f, 0.12f, 0.05f}, {-0.86f, 0.12f, 0.07f}, wire, 4);
            addSegment(cloud, {-0.86f, -0.12f, 0.05f}, {-0.86f, -0.12f, 0.07f}, wire, 4);

            std::vector<glm::vec3> resistor;
            resistor.emplace_back(-0.34f, top, 0.06f);
            for (int index = 1; index <= 10; ++index) {
                const float t = static_cast<float>(index) / 10.0f;
                const float x = glm::mix(-0.34f, 0.34f, t);
                const float y = top + (index % 2 == 0 ? -0.095f : 0.095f);
                resistor.emplace_back(x, y, 0.06f);
            }
            resistor.emplace_back(0.34f, top, 0.06f);
            addPolyline(cloud, resistor, GOLD, 16);

            // The lamp is a load in series with the battery and resistor.
            addCircle(cloud, {right, 0.0f, 0.09f}, 0.20f, 0.09f, GOLD, 260);
            addSegment(cloud, {right - 0.11f, -0.11f, 0.10f}, {right + 0.11f, 0.11f, 0.10f}, PALE, 24);
            addSegment(cloud, {right - 0.11f, 0.11f, 0.10f}, {right + 0.11f, -0.11f, 0.10f}, PALE, 24);
            addSphere(cloud, {right, 0.0f, 0.02f}, 0.075f, glm::vec4(1.0f, 0.80f, 0.28f, 0.72f), 420);

            // Conventional current and electron drift share the same wire
            // segment and point in opposite directions.
            addArrow(cloud, {0.43f, 0.64f, 0.10f}, {0.78f, 0.64f, 0.10f}, CYAN, 0.065f);
            addArrow(cloud, {0.78f, 0.46f, 0.10f}, {0.43f, 0.46f, 0.10f}, PALE, 0.065f);
        }

        void buildCarbonAtom(PointCloud& cloud) {
            const glm::vec3 nucleus(0.0f, 0.0f, 0.04f);
            addSphere(cloud, nucleus, 0.21f, glm::vec4(0.63f, 0.20f, 0.68f, 0.46f), 1800);

            // Carbon-12 has six protons and six neutrons; each is kept distinct.
            for (int particle = 0; particle < 12; ++particle) {
                const glm::vec3 direction = sphereDirection(
                    static_cast<std::size_t>(particle),
                    12
                );
                const glm::vec3 center = nucleus + direction * 0.105f;
                addSphere(
                    cloud,
                    center,
                    0.064f,
                    particle % 2 == 0
                        ? glm::vec4(1.0f, 0.28f, 0.30f, 0.96f)
                        : glm::vec4(0.75f, 0.84f, 0.94f, 0.94f),
                    260
                );
            }

            // Bohr-style shells are only a teaching scaffold; these are not
            // classical electron trajectories or literal circular orbits.
            addCircle(cloud, nucleus, 0.40f, 0.04f, glm::vec4(0.26f, 0.58f, 0.80f, 0.34f), 360);
            addCircle(cloud, nucleus, 0.83f, 0.035f, glm::vec4(0.22f, 0.51f, 0.72f, 0.27f), 480);
            addCharge(cloud, {-0.40f, 0.0f, 0.08f}, CYAN);
            addCharge(cloud, {0.40f, 0.0f, 0.08f}, CYAN);
            addCharge(cloud, {0.0f, 0.83f, 0.08f}, CYAN);
            addCharge(cloud, {0.83f, 0.0f, 0.08f}, CYAN);
            addCharge(cloud, {0.0f, -0.83f, 0.08f}, CYAN);
            addCharge(cloud, {-0.83f, 0.0f, 0.08f}, CYAN);
        }

        void buildWaterCycle(PointCloud& cloud) {
            addSphere(cloud, {-0.91f, 0.78f, -0.02f}, 0.16f, GOLD, 1400);
            addGridPatch(
                cloud, -1.30f, -0.95f, -0.08f, -0.56f, -0.02f,
                glm::vec4(0.03f, 0.34f, 0.78f, 0.66f), 42, 14
            );
            addSegment(cloud, {-1.30f, -0.54f, 0.0f}, {-0.08f, -0.54f, 0.0f}, CYAN, 70);

            // A raised land profile and mountain, drawn as a cutaway surface.
            addQuadSurface(
                cloud,
                {-0.06f, -0.56f, -0.04f},
                {1.24f, -0.56f, -0.04f},
                {1.24f, -0.24f, -0.04f},
                {-0.06f, -0.45f, -0.04f},
                glm::vec4(0.24f, 0.43f, 0.24f, 0.68f),
                36
            );
            addQuadSurface(
                cloud,
                {0.25f, -0.44f, 0.0f},
                {1.22f, -0.27f, 0.0f},
                {0.76f, 0.45f, 0.0f},
                {0.36f, -0.09f, 0.0f},
                glm::vec4(0.34f, 0.52f, 0.31f, 0.60f),
                34
            );
            addPolyline(
                cloud,
                {{0.25f, -0.44f, 0.04f}, {0.52f, -0.25f, 0.04f},
                 {0.76f, 0.45f, 0.04f}, {0.96f, -0.26f, 0.04f},
                 {1.22f, -0.27f, 0.04f}},
                glm::vec4(0.64f, 0.81f, 0.70f, 0.88f),
                26
            );

            // Cloud puffs form one condensation region rather than extra layers.
            const glm::vec4 cloudColor(0.68f, 0.86f, 0.98f, 0.72f);
            addSphere(cloud, {0.03f, 0.71f, 0.02f}, {0.22f, 0.14f, 0.09f}, cloudColor, 1200);
            addSphere(cloud, {0.23f, 0.79f, 0.02f}, {0.20f, 0.18f, 0.10f}, cloudColor, 1300);
            addSphere(cloud, {0.44f, 0.71f, 0.02f}, {0.21f, 0.13f, 0.09f}, cloudColor, 1200);

            // A plant makes transpiration part of the water cycle diagram.
            addTube(cloud, {1.03f, -0.30f, 0.09f}, {1.03f, -0.05f, 0.09f}, 0.018f, GREEN, 28, 6);
            addSphere(cloud, {0.94f, -0.11f, 0.09f}, {0.10f, 0.042f, 0.028f}, GREEN, 320);
            addSphere(cloud, {1.11f, -0.16f, 0.09f}, {0.09f, 0.038f, 0.025f}, GREEN, 320);

            addArrow(cloud, {-0.58f, -0.43f, 0.12f}, {-0.36f, 0.46f, 0.12f}, CYAN, 0.09f);
            addArrow(cloud, {0.50f, 0.53f, 0.12f}, {0.72f, -0.18f, 0.12f}, BLUE, 0.085f);
            addArrow(cloud, {0.87f, -0.42f, 0.12f}, {0.30f, -0.48f, 0.12f}, CYAN, 0.08f);
            addArrow(cloud, {1.03f, -0.05f, 0.13f}, {0.88f, 0.45f, 0.13f}, GREEN, 0.07f);
            addArrow(cloud, {0.78f, -0.36f, 0.13f}, {0.78f, -0.69f, 0.13f}, BLUE, 0.065f);
            addBezierLine(
                cloud,
                {0.96f, -0.61f, 0.06f},
                {0.57f, -0.83f, 0.06f},
                {0.14f, -0.91f, 0.06f},
                {-0.14f, -0.78f, 0.06f},
                glm::vec4(0.20f, 0.68f, 0.94f, 0.76f),
                64
            );
            addArrow(cloud, {0.26f, -0.83f, 0.13f}, {-0.02f, -0.80f, 0.13f}, BLUE, 0.06f);
        }

        void buildPlateBoundaries(PointCloud& cloud) {
            const glm::vec4 crust(0.55f, 0.28f, 0.16f, 0.84f);
            const glm::vec4 ocean(0.12f, 0.42f, 0.72f, 0.78f);
            const glm::vec4 mantle(0.82f, 0.30f, 0.12f, 0.34f);
            addGridPatch(cloud, -1.40f, -0.58f, 1.40f, -0.34f, -0.12f, mantle, 80, 10);

            // Divergent ridge: plates move apart and magma rises to make crust.
            addGridPatch(cloud, -1.36f, -0.10f, -0.92f, 0.14f, 0.0f, ocean, 24, 10);
            addGridPatch(cloud, -0.50f, -0.10f, -0.08f, 0.14f, 0.0f, ocean, 20, 10);
            addTube(cloud, {-0.71f, -0.30f, 0.08f}, {-0.71f, 0.18f, 0.08f}, 0.018f, GOLD, 48, 6);
            addArrow(cloud, {-0.71f, 0.29f, 0.10f}, {-1.02f, 0.29f, 0.10f}, CYAN, 0.06f);
            addArrow(cloud, {-0.71f, 0.29f, 0.10f}, {-0.40f, 0.29f, 0.10f}, CYAN, 0.06f);

            // Convergent oceanic-continental boundary with one subducting plate.
            addGridPatch(cloud, 0.10f, -0.10f, 0.61f, 0.14f, 0.0f, crust, 24, 10);
            addPolyline(
                cloud,
                {{-0.02f, -0.10f, 0.05f}, {0.18f, -0.20f, 0.05f},
                 {0.37f, -0.36f, 0.05f}, {0.58f, -0.48f, 0.05f}},
                ocean,
                34
            );
            addArrow(cloud, {-0.27f, 0.25f, 0.10f}, {0.06f, 0.25f, 0.10f}, CYAN, 0.055f);
            addTube(cloud, {0.10f, 0.14f, 0.08f}, {0.10f, -0.07f, 0.08f}, 0.018f, PALE, 26, 5);
            addArrow(cloud, {0.46f, 0.28f, 0.10f}, {0.10f, 0.28f, 0.10f}, CYAN, 0.06f);
            addPolyline(
                cloud,
                {{0.33f, 0.14f, 0.10f}, {0.42f, 0.26f, 0.10f},
                 {0.50f, 0.39f, 0.10f}, {0.54f, 0.50f, 0.10f}},
                RED,
                24
            );
            addArrow(cloud, {0.78f, 0.50f, 0.10f}, {0.78f, 0.19f, 0.10f}, GOLD, 0.06f);

            // Transform boundary: adjacent crustal blocks slide in opposite directions.
            addGridPatch(cloud, 0.82f, -0.10f, 1.36f, 0.14f, 0.0f, crust, 24, 10);
            addTube(cloud, {0.79f, -0.24f, 0.08f}, {0.79f, 0.28f, 0.08f}, 0.012f, PALE, 40, 5);
            addArrow(cloud, {1.07f, -0.02f, 0.10f}, {1.07f, 0.34f, 0.10f}, CYAN, 0.06f);
            addArrow(cloud, {1.25f, 0.20f, 0.10f}, {1.25f, -0.16f, 0.10f}, CYAN, 0.06f);
        }

        void buildMarineFoodWeb(PointCloud& cloud) {
            const glm::vec3 sun(0.0f, 0.94f, 0.02f);
            const glm::vec3 producers(-0.82f, 0.33f, 0.04f);
            const glm::vec3 zooplankton(-0.23f, -0.02f, 0.04f);
            const glm::vec3 smallFish(0.47f, 0.34f, 0.04f);
            const glm::vec3 largeFish(0.82f, -0.34f, 0.04f);
            const glm::vec3 decomposers(-0.52f, -0.55f, 0.04f);

            addSphere(cloud, sun, 0.12f, GOLD, 1200);
            addCircleNode(cloud, producers, 0.20f, GREEN);
            addCircleNode(cloud, zooplankton, 0.17f, glm::vec4(0.30f, 0.79f, 0.94f, 0.9f));
            addFish(cloud, smallFish, 0.38f, 0.19f, glm::vec4(0.94f, 0.67f, 0.24f, 0.92f));
            addFish(cloud, largeFish, 0.49f, 0.24f, glm::vec4(0.98f, 0.35f, 0.26f, 0.94f));
            addCircleNode(cloud, decomposers, 0.15f, glm::vec4(0.72f, 0.43f, 0.90f, 0.88f));

            addArrow(cloud, {-0.08f, 0.77f, 0.12f}, {-0.66f, 0.49f, 0.12f}, GOLD, 0.075f);
            addArrow(cloud, {-0.60f, 0.23f, 0.13f}, {-0.37f, 0.08f, 0.13f}, CYAN, 0.07f);
            addArrow(cloud, {-0.05f, 0.04f, 0.13f}, {0.28f, 0.26f, 0.13f}, CYAN, 0.07f);
            addArrow(cloud, {0.57f, 0.17f, 0.13f}, {0.72f, -0.10f, 0.13f}, CYAN, 0.07f);
            addArrow(cloud, {0.61f, -0.43f, 0.13f}, {-0.34f, -0.58f, 0.13f}, PALE, 0.07f);
            addArrow(cloud, {-0.55f, -0.37f, 0.13f}, {-0.75f, 0.10f, 0.13f}, GREEN, 0.07f);
        }

        void buildPythagoreanDiagram(PointCloud& cloud) {
            constexpr float scale = 0.2333333333f;
            const glm::vec3 a(-0.58f, -0.55f, 0.04f);
            const glm::vec3 b(a.x + 3.0f * scale, a.y, 0.04f);
            const glm::vec3 c(a.x, a.y + 4.0f * scale, 0.04f);
            const glm::vec4 triangleColor(0.89f, 0.93f, 1.0f, 0.90f);
            const glm::vec4 sideA(0.12f, 0.70f, 1.0f, 0.92f);
            const glm::vec4 sideB(0.30f, 0.91f, 0.60f, 0.92f);
            const glm::vec4 sideC(1.0f, 0.66f, 0.19f, 0.95f);

            // Right triangle is drawn at exact 3:4:5 proportions.
            for (int row = 0; row <= 48; ++row) {
                const float v = static_cast<float>(row) / 48.0f;
                for (int column = 0; column <= 48; ++column) {
                    const float u = static_cast<float>(column) / 48.0f;
                    if (u + v <= 1.001f) {
                        cloud.addPoint(
                            a + (b - a) * u + (c - a) * v,
                            glm::vec4(0.30f, 0.38f, 0.50f, 0.43f)
                        );
                    }
                }
            }
            addSegment(cloud, a, b, sideA, 70, 1.2f);
            addSegment(cloud, a, c, sideB, 70, 1.2f);
            addSegment(cloud, b, c, sideC, 80, 1.2f);

            const auto addSquare = [&](
                const glm::vec3& start,
                const glm::vec3& end,
                float normalSign,
                const glm::vec4& color
            ) {
                const glm::vec2 direction = glm::normalize(
                    glm::vec2(end.x - start.x, end.y - start.y)
                );
                const glm::vec2 outward(
                    direction.y * normalSign,
                    -direction.x * normalSign
                );
                const float length = glm::length(end - start);
                const glm::vec3 offset(outward.x * length, outward.y * length, 0.0f);
                addClosedSquare(
                    cloud,
                    {{start, end, end + offset, start + offset}},
                    color,
                    24
                );
            };
            addSquare(a, b, 1.0f, sideA);
            addSquare(a, c, -1.0f, sideB);
            addSquare(b, c, 1.0f, sideC);

            // Right-angle marker at the vertex where the two legs meet.
            constexpr float tick = 0.09f;
            addPolyline(
                cloud,
                {a + glm::vec3(tick, 0.0f, 0.12f),
                 a + glm::vec3(tick, tick, 0.12f),
                 a + glm::vec3(0.0f, tick, 0.12f)},
                PALE,
                18
            );
        }

        void buildColdFront(PointCloud& cloud) {
            const glm::vec4 coldAir(0.08f, 0.38f, 0.84f, 0.42f);
            const glm::vec4 warmAir(0.96f, 0.36f, 0.10f, 0.28f);
            const glm::vec3 groundLeft(-1.25f, -0.66f, 0.0f);
            const glm::vec3 groundRight(1.25f, -0.66f, 0.0f);
            addGridPatch(cloud, -1.25f, -0.60f, 0.0f, 0.07f, -0.05f, coldAir, 42, 24);
            addGridPatch(cloud, 0.0f, -0.60f, 1.25f, 0.50f, -0.05f, warmAir, 42, 32);
            addSegment(cloud, groundLeft, groundRight, PALE, 110, 1.1f);

            // Advancing dense air wedges beneath warm, moist air and lifts it.
            addPolyline(
                cloud,
                {{-0.08f, -0.60f, 0.07f}, {0.14f, -0.26f, 0.07f},
                 {0.36f, 0.05f, 0.07f}, {0.58f, 0.38f, 0.07f}},
                BLUE,
                30
            );
            addArrow(cloud, {-0.92f, -0.36f, 0.09f}, {-0.31f, -0.36f, 0.09f}, BLUE, 0.095f);
            addBezierLine(
                cloud,
                {0.92f, -0.31f, 0.10f},
                {0.78f, 0.12f, 0.10f},
                {0.44f, 0.50f, 0.10f},
                {0.21f, 0.91f, 0.10f},
                glm::vec4(1.0f, 0.61f, 0.22f, 0.9f),
                96
            );
            addArrow(cloud, {0.87f, -0.25f, 0.11f}, {0.67f, 0.12f, 0.11f}, GOLD, 0.08f);

            const glm::vec4 cloudColor(0.70f, 0.84f, 0.98f, 0.72f);
            addSphere(cloud, {0.20f, 0.93f, 0.04f}, {0.23f, 0.13f, 0.08f}, cloudColor, 1200);
            addSphere(cloud, {0.43f, 0.99f, 0.04f}, {0.20f, 0.15f, 0.09f}, cloudColor, 1200);
            addSphere(cloud, {0.63f, 0.91f, 0.04f}, {0.20f, 0.11f, 0.07f}, cloudColor, 1000);
            for (int drop = 0; drop < 7; ++drop) {
                const float x = 0.25f + static_cast<float>(drop) * 0.08f;
                const float top = 0.77f - static_cast<float>(drop % 3) * 0.035f;
                addSegment(
                    cloud,
                    {x, top, 0.14f},
                    {x - 0.06f, top - 0.22f, 0.14f},
                    CYAN,
                    20,
                    1.2f
                );
            }
        }

        void addTransparentAnchors(PointCloud& cloud) {
            while (cloud.getPointCount() < MORPH_PARTICLE_COUNT) {
                cloud.addPoint(glm::vec3(0.0f), glm::vec4(0.0f));
            }
        }

    }

    bool TestLesson::recognizesTranscript(
        std::string_view transcript
    ) const {
        const std::string text = lowercase(transcript);
        return
            (text.find("circuit") != std::string::npos &&
             text.find("battery") != std::string::npos &&
             text.find("resistor") != std::string::npos) ||
            (text.find("carbon-12") != std::string::npos &&
             text.find("proton") != std::string::npos &&
             text.find("electron") != std::string::npos) ||
            (text.find("water cycle") != std::string::npos &&
             text.find("evaporation") != std::string::npos &&
             text.find("condens") != std::string::npos) ||
            (text.find("plate") != std::string::npos &&
             text.find("divergent") != std::string::npos &&
             text.find("convergent") != std::string::npos &&
             text.find("transform") != std::string::npos) ||
            (text.find("food web") != std::string::npos &&
             text.find("producer") != std::string::npos &&
             text.find("decomposer") != std::string::npos) ||
            (text.find("pythagorean") != std::string::npos &&
             text.find("hypotenuse") != std::string::npos) ||
            (text.find("cold front") != std::string::npos &&
             text.find("warm air") != std::string::npos);
    }

    bool TestLesson::applyTranscript(std::string_view transcript) {
        const std::string text = lowercase(transcript);
        TestLessonTopic detected = TestLessonTopic::None;
        if (
            text.find("circuit") != std::string::npos &&
            text.find("battery") != std::string::npos &&
            text.find("resistor") != std::string::npos
        ) {
            detected = TestLessonTopic::ElectricCircuit;
        }
        else if (
            text.find("carbon-12") != std::string::npos &&
            text.find("proton") != std::string::npos &&
            text.find("electron") != std::string::npos
        ) {
            detected = TestLessonTopic::CarbonAtom;
        }
        else if (
            text.find("water cycle") != std::string::npos &&
            text.find("evaporation") != std::string::npos &&
            text.find("condens") != std::string::npos
        ) {
            detected = TestLessonTopic::WaterCycle;
        }
        else if (
            text.find("plate") != std::string::npos &&
            text.find("divergent") != std::string::npos &&
            text.find("convergent") != std::string::npos &&
            text.find("transform") != std::string::npos
        ) {
            detected = TestLessonTopic::PlateBoundaries;
        }
        else if (
            text.find("food web") != std::string::npos &&
            text.find("producer") != std::string::npos &&
            text.find("decomposer") != std::string::npos
        ) {
            detected = TestLessonTopic::MarineFoodWeb;
        }
        else if (
            text.find("pythagorean") != std::string::npos &&
            text.find("hypotenuse") != std::string::npos
        ) {
            detected = TestLessonTopic::PythagoreanTheorem;
        }
        else if (
            text.find("cold front") != std::string::npos &&
            text.find("warm air") != std::string::npos
        ) {
            detected = TestLessonTopic::ColdFront;
        }

        if (detected == TestLessonTopic::None || detected == topic_) {
            return false;
        }
        topic_ = detected;
        return true;
    }

    void TestLesson::clear() {
        topic_ = TestLessonTopic::None;
    }

    bool TestLesson::hasLesson() const {
        return topic_ != TestLessonTopic::None;
    }

    TestLessonTopic TestLesson::getTopic() const {
        return topic_;
    }

    const char* TestLesson::getTitle() const {
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit: return "PHYSICS  /  SERIES CIRCUIT";
            case TestLessonTopic::CarbonAtom: return "CHEMISTRY  /  CARBON-12";
            case TestLessonTopic::WaterCycle: return "EARTH SCIENCE  /  WATER CYCLE";
            case TestLessonTopic::PlateBoundaries: return "GEOLOGY  /  PLATE BOUNDARIES";
            case TestLessonTopic::MarineFoodWeb: return "ECOLOGY  /  MARINE FOOD WEB";
            case TestLessonTopic::PythagoreanTheorem: return "MATHEMATICS  /  PYTHAGOREAN THEOREM";
            case TestLessonTopic::ColdFront: return "METEOROLOGY  /  COLD FRONT";
            default: return "SCIENCE LESSON";
        }
    }

    const char* TestLesson::getSubtitle() const {
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit:
                return "CLOSED PATH  /  SERIES CURRENT  /  ELECTRONS OPPOSITE CONVENTIONAL CURRENT";
            case TestLessonTopic::CarbonAtom:
                return "SIX PROTONS  /  SIX NEUTRONS  /  SIX ELECTRONS  /  SCHEMATIC SHELL MODEL";
            case TestLessonTopic::WaterCycle:
                return "SOLAR ENERGY AND GRAVITY MOVE WATER BETWEEN RESERVOIRS";
            case TestLessonTopic::PlateBoundaries:
                return "DIVERGENT  /  CONVERGENT  /  TRANSFORM  ·  SCHEMATICS NOT TO SCALE";
            case TestLessonTopic::MarineFoodWeb:
                return "ARROWS SHOW ENERGY FLOW FROM FOOD TO CONSUMER";
            case TestLessonTopic::PythagoreanTheorem:
                return "AREAS ON THE LEGS SUM TO THE AREA ON THE HYPOTENUSE";
            case TestLessonTopic::ColdFront:
                return "DENSE COLD AIR LIFTS WARM MOIST AIR  /  CONDITIONS VARY";
            default:
                return "SCHEMATIC EDUCATIONAL MODEL";
        }
    }

    const char* TestLesson::getDescription() const {
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit:
                return "One closed series loop contains a battery, resistor, and lamp. The cyan arrow shows conventional current; the moving charge marker shows electron drift in the opposite direction along the external circuit.";
            case TestLessonTopic::CarbonAtom:
                return "Carbon-12 nucleus with six protons and six neutrons, plus six electrons shown in a simplified Bohr-style shell diagram. Real electron states are quantum probability distributions, not fixed circular paths.";
            case TestLessonTopic::WaterCycle:
                return "Ocean, sunlight, evaporation, condensation, clouds, precipitation, runoff, plant transpiration, and a groundwater path. The arrows illustrate major transfers, not quantities or scale.";
            case TestLessonTopic::PlateBoundaries:
                return "Two simplified cross-sections and one plan view: plates move apart and create crust at a divergent ridge, oceanic lithosphere subducts beneath continental crust at a convergent boundary, and crust slides laterally along a transform fault. Not to scale.";
            case TestLessonTopic::MarineFoodWeb:
                return "A simplified marine web links sunlight, phytoplankton, zooplankton, fish, and decomposers. Arrows point from food to consumer; nutrients recycle, while energy flows through and leaves as heat.";
            case TestLessonTopic::PythagoreanTheorem:
                return "A right triangle with 3:4:5 side lengths and a square constructed outward on each side. Their areas are 9, 16, and 25 square units.";
            case TestLessonTopic::ColdFront:
                return "Cross-section of advancing cold air wedging beneath warm, moist air and lifting it. Cloud and rain symbols show one possible outcome when moisture and instability are sufficient, not a forecast.";
            default:
                return "Choose a temporary test preset to preview a lesson scene.";
        }
    }

    const char* TestLesson::getFeedback() const {
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit:
                return "Physics lesson built: trace the closed path and compare current with electron drift.";
            case TestLessonTopic::CarbonAtom:
                return "Chemistry lesson built: inspect the carbon-12 nucleus and schematic electron shells.";
            case TestLessonTopic::WaterCycle:
                return "Earth-science lesson built: follow water through atmosphere, land, ocean, and groundwater.";
            case TestLessonTopic::PlateBoundaries:
                return "Geology lesson built: compare divergent, convergent, and transform motion.";
            case TestLessonTopic::MarineFoodWeb:
                return "Ecology lesson built: follow energy through a marine food web and nutrients back to producers.";
            case TestLessonTopic::PythagoreanTheorem:
                return "Mathematics lesson built: compare the areas on the three sides of a 3-4-5 triangle.";
            case TestLessonTopic::ColdFront:
                return "Meteorology lesson built: watch dense cold air lift warm, moist air.";
            default:
                return "Choose a temporary test preset.";
        }
    }

    PointCloud TestLesson::buildPointCloud() const {
        PointCloud cloud(getTitle());
        cloud.reserve(MORPH_PARTICLE_COUNT);
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit:
                buildCircuit(cloud);
                break;
            case TestLessonTopic::CarbonAtom:
                buildCarbonAtom(cloud);
                break;
            case TestLessonTopic::WaterCycle:
                buildWaterCycle(cloud);
                break;
            case TestLessonTopic::PlateBoundaries:
                buildPlateBoundaries(cloud);
                break;
            case TestLessonTopic::MarineFoodWeb:
                buildMarineFoodWeb(cloud);
                break;
            case TestLessonTopic::PythagoreanTheorem:
                buildPythagoreanDiagram(cloud);
                break;
            case TestLessonTopic::ColdFront:
                buildColdFront(cloud);
                break;
            default:
                return cloud;
        }
        addTransparentAnchors(cloud);
        return cloud;
    }

    std::vector<LessonLabel> TestLesson::getLabels() const {
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit:
                return {
                    {"Battery", {-1.03f, 0.27f, 0.11f}, {1.0f, 0.75f, 0.25f, 1.0f}},
                    {"Resistor", {0.0f, 0.78f, 0.12f}, {1.0f, 0.75f, 0.25f, 1.0f}},
                    {"Lamp / load", {1.12f, 0.08f, 0.16f}, {1.0f, 0.75f, 0.25f, 1.0f}},
                    {"Conventional current", {0.60f, 0.76f, 0.12f}, {0.18f, 0.78f, 1.0f, 1.0f}},
                    {"Electron drift", {0.60f, 0.34f, 0.12f}, {0.69f, 0.88f, 1.0f, 1.0f}}
                };
            case TestLessonTopic::CarbonAtom:
                return {
                    {"Nucleus: 6p + 6n", {-0.02f, 0.10f, 0.28f}, {0.90f, 0.46f, 0.91f, 1.0f}},
                    {"Six electrons", {0.86f, 0.71f, 0.12f}, {0.20f, 0.83f, 1.0f, 1.0f}},
                    {"2 | 4 shell occupancy", {-0.72f, -0.76f, 0.12f}, {0.44f, 0.71f, 0.88f, 1.0f}}
                };
            case TestLessonTopic::WaterCycle:
                return {
                    {"Sunlight", {-0.96f, 0.98f, 0.16f}, {1.0f, 0.78f, 0.30f, 1.0f}},
                    {"Evaporation", {-0.48f, 0.03f, 0.16f}, {0.30f, 0.83f, 1.0f, 1.0f}},
                    {"Condensation", {0.21f, 0.98f, 0.16f}, {0.70f, 0.87f, 1.0f, 1.0f}},
                    {"Precipitation", {0.76f, 0.45f, 0.16f}, {0.31f, 0.69f, 1.0f, 1.0f}},
                    {"Runoff", {0.78f, -0.43f, 0.16f}, {0.31f, 0.76f, 1.0f, 1.0f}},
                    {"Groundwater", {0.41f, -0.89f, 0.16f}, {0.35f, 0.69f, 0.93f, 1.0f}},
                    {"Transpiration", {1.13f, -0.03f, 0.16f}, {0.41f, 0.94f, 0.58f, 1.0f}}
                };
            case TestLessonTopic::PlateBoundaries:
                return {
                    {"Divergent ridge", {-0.34f, 0.43f, 0.14f}, {0.33f, 0.84f, 1.0f, 1.0f}},
                    {"New crust", {-0.34f, -0.20f, 0.14f}, {1.0f, 0.76f, 0.34f, 1.0f}},
                    {"Convergent boundary", {0.35f, 0.46f, 0.14f}, {1.0f, 0.75f, 0.36f, 1.0f}},
                    {"Subducting plate", {0.37f, -0.33f, 0.14f}, {0.45f, 0.78f, 1.0f, 1.0f}},
                    {"Transform fault", {1.05f, 0.42f, 0.14f}, {0.50f, 0.91f, 0.75f, 1.0f}}
                };
            case TestLessonTopic::MarineFoodWeb:
                return {
                    {"Sunlight", {0.0f, 1.12f, 0.16f}, {1.0f, 0.78f, 0.30f, 1.0f}},
                    {"Phytoplankton (producer)", {-0.98f, 0.39f, 0.17f}, {0.48f, 0.96f, 0.62f, 1.0f}},
                    {"Zooplankton", {-0.40f, -0.10f, 0.17f}, {0.48f, 0.87f, 1.0f, 1.0f}},
                    {"Small fish", {0.43f, 0.58f, 0.17f}, {1.0f, 0.82f, 0.40f, 1.0f}},
                    {"Large fish", {1.03f, -0.31f, 0.17f}, {1.0f, 0.48f, 0.37f, 1.0f}},
                    {"Decomposers / nutrients", {-0.63f, -0.79f, 0.17f}, {0.81f, 0.61f, 1.0f, 1.0f}}
                };
            case TestLessonTopic::PythagoreanTheorem:
                return {
                    {"a = 3  |  area 9", {-0.20f, -0.89f, 0.14f}, {0.32f, 0.78f, 1.0f, 1.0f}},
                    {"b = 4  |  area 16", {-1.37f, -0.02f, 0.14f}, {0.38f, 0.95f, 0.65f, 1.0f}},
                    {"c = 5  |  area 25", {0.72f, 0.70f, 0.14f}, {1.0f, 0.73f, 0.29f, 1.0f}},
                    {"a^2 + b^2 = c^2", {0.10f, -0.95f, 0.14f}, {0.92f, 0.96f, 1.0f, 1.0f}}
                };
            case TestLessonTopic::ColdFront:
                return {
                    {"Advancing cold, dense air", {-0.93f, -0.11f, 0.16f}, {0.40f, 0.73f, 1.0f, 1.0f}},
                    {"Warm, moist air", {1.03f, -0.01f, 0.16f}, {1.0f, 0.61f, 0.30f, 1.0f}},
                    {"Rapid uplift", {0.62f, 0.43f, 0.16f}, {1.0f, 0.79f, 0.43f, 1.0f}},
                    {"Clouds", {0.44f, 1.14f, 0.16f}, {0.77f, 0.91f, 1.0f, 1.0f}},
                    {"Showers possible", {0.79f, 0.50f, 0.16f}, {0.38f, 0.80f, 1.0f, 1.0f}}
                };
            default:
                return {};
        }
    }

    std::optional<LessonAnimationMarker> TestLesson::getAnimationMarker(
        float animationTime
    ) const {
        switch (topic_) {
            case TestLessonTopic::ElectricCircuit: {
                constexpr std::array<glm::vec3, 6> route = {{
                    {-0.86f, -0.12f, 0.15f},
                    {-0.86f, -0.55f, 0.15f},
                    {0.86f, -0.55f, 0.15f},
                    {0.86f, 0.20f, 0.15f},
                    {0.86f, 0.55f, 0.15f},
                    {-0.86f, 0.55f, 0.15f}
                }};
                return LessonAnimationMarker{
                    routePosition(route, animationTime, 8.0f),
                    glm::vec4(0.28f, 0.83f, 1.0f, 1.0f)
                };
            }
            case TestLessonTopic::WaterCycle: {
                constexpr std::array<glm::vec3, 5> route = {{
                    {-0.55f, -0.49f, 0.17f},
                    {-0.35f, 0.52f, 0.17f},
                    {0.26f, 0.68f, 0.17f},
                    {0.73f, -0.12f, 0.17f},
                    {0.20f, -0.66f, 0.17f}
                }};
                return LessonAnimationMarker{
                    routePosition(route, animationTime, 10.0f),
                    glm::vec4(0.40f, 0.87f, 1.0f, 1.0f)
                };
            }
            case TestLessonTopic::MarineFoodWeb: {
                constexpr std::array<glm::vec3, 5> route = {{
                    {-0.08f, 0.79f, 0.16f},
                    {-0.65f, 0.43f, 0.16f},
                    {-0.28f, 0.02f, 0.16f},
                    {0.39f, 0.29f, 0.16f},
                    {0.78f, -0.22f, 0.16f}
                }};
                return LessonAnimationMarker{
                    routePosition(route, animationTime, 9.0f),
                    glm::vec4(1.0f, 0.79f, 0.28f, 1.0f)
                };
            }
            case TestLessonTopic::ColdFront: {
                constexpr std::array<glm::vec3, 4> route = {{
                    {0.94f, -0.30f, 0.17f},
                    {0.75f, 0.08f, 0.17f},
                    {0.45f, 0.48f, 0.17f},
                    {0.22f, 0.83f, 0.17f}
                }};
                return LessonAnimationMarker{
                    routePosition(route, animationTime, 6.0f),
                    glm::vec4(1.0f, 0.68f, 0.30f, 1.0f)
                };
            }
            default:
                return std::nullopt;
        }
    }

}
