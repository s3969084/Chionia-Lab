#include "infinity_generator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace chionia {

    namespace {

        constexpr std::size_t CURVE_SEGMENT_COUNT = 8192;
        constexpr double TWO_PI = 6.28318530717958647692;

        struct ArcSample {
            glm::vec3 position;
            double cumulativeLength;
        };

        glm::vec3 evaluateLemniscate(double parameter) {
            const double sine = std::sin(parameter);
            const double cosine = std::cos(parameter);
            const double denominator = 1.0 + sine * sine;

            return glm::vec3(
                static_cast<float>(cosine / denominator),
                static_cast<float>(sine * cosine / denominator),
                0.0f
            );
        }

        double radicalInverse(std::size_t index, std::size_t base) {
            double result = 0.0;
            double fraction = 1.0 / static_cast<double>(base);

            while (index > 0) {
                result +=
                    static_cast<double>(index % base) * fraction;
                index /= base;
                fraction /= static_cast<double>(base);
            }

            return result;
        }

        std::vector<ArcSample> buildArcTable(double& totalLength) {
            std::vector<ArcSample> samples;
            samples.reserve(CURVE_SEGMENT_COUNT + 1);

            glm::vec3 previousPosition = evaluateLemniscate(0.0);
            double cumulativeLength = 0.0;
            samples.push_back({previousPosition, cumulativeLength});

            for (
                std::size_t i = 1;
                i <= CURVE_SEGMENT_COUNT;
                ++i
            ) {
                const double parameter =
                    TWO_PI * static_cast<double>(i) /
                    static_cast<double>(CURVE_SEGMENT_COUNT);

                const glm::vec3 position =
                    evaluateLemniscate(parameter);

                const glm::vec3 delta = position - previousPosition;
                cumulativeLength += static_cast<double>(glm::length(delta));
                samples.push_back({position, cumulativeLength});
                previousPosition = position;
            }

            totalLength = cumulativeLength;
            return samples;
        }

        glm::vec3 sampleByArcLength(
            const std::vector<ArcSample>& arcTable,
            double distance,
            double& parameter
        ) {
            const auto upper = std::lower_bound(
                arcTable.begin() + 1,
                arcTable.end(),
                distance,
                [](const ArcSample& sample, double value) {
                    return sample.cumulativeLength < value;
                }
            );

            const std::size_t upperIndex = static_cast<std::size_t>(
                std::distance(arcTable.begin(), upper)
            );

            if (upperIndex >= arcTable.size()) {
                parameter = TWO_PI;
                return arcTable.back().position;
            }

            const ArcSample& start = arcTable[upperIndex - 1];
            const ArcSample& end = arcTable[upperIndex];
            const double segmentLength =
                end.cumulativeLength - start.cumulativeLength;

            const double fraction = segmentLength > 0.0
                ? (distance - start.cumulativeLength) / segmentLength
                : 0.0;

            parameter = TWO_PI *
                (static_cast<double>(upperIndex - 1) + fraction) /
                static_cast<double>(CURVE_SEGMENT_COUNT);

            return glm::mix(
                start.position,
                end.position,
                static_cast<float>(fraction)
            );
        }

    }

    PointCloud InfinityGenerator::generate(
        const InfinityGeneratorConfig& config
    ) {
        if (config.pointCount < 3) {
            throw std::invalid_argument(
                "Infinity point count must be at least three."
            );
        }

        if (!std::isfinite(config.thickness) || config.thickness < 0.0f) {
            throw std::invalid_argument(
                "Infinity thickness must be finite and non-negative."
            );
        }

        double totalLength = 0.0;
        const std::vector<ArcSample> arcTable =
            buildArcTable(totalLength);

        if (!std::isfinite(totalLength) || totalLength <= 0.0) {
            throw std::runtime_error(
                "Failed to calculate Infinity curve length."
            );
        }

        PointCloud pointCloud("Infinity");
        pointCloud.reserve(config.pointCount);

        const glm::vec3 depthAxis(0.0f, 0.0f, 1.0f);
        const double tangentStep =
            TWO_PI / static_cast<double>(CURVE_SEGMENT_COUNT);

        for (std::size_t i = 0; i < config.pointCount; ++i) {
            const double alongCurve =
                (static_cast<double>(i) + 0.5) /
                static_cast<double>(config.pointCount);

            double parameter = 0.0;
            const glm::vec3 centerlinePosition = sampleByArcLength(
                arcTable,
                alongCurve * totalLength,
                parameter
            );

            const glm::vec3 tangent = glm::normalize(
                evaluateLemniscate(parameter + tangentStep) -
                evaluateLemniscate(parameter - tangentStep)
            );

            const glm::vec3 curveNormal = glm::normalize(
                glm::vec3(-tangent.y, tangent.x, 0.0f)
            );

            const double radialSample = radicalInverse(i + 1, 2);
            const double angularSample = radicalInverse(i + 1, 3);
            const float radius = config.thickness * static_cast<float>(
                std::sqrt(radialSample)
            );
            const double angle = TWO_PI * angularSample;

            const glm::vec3 tubeOffset = radius * (
                curveNormal * static_cast<float>(std::cos(angle)) +
                depthAxis * static_cast<float>(std::sin(angle))
            );

            pointCloud.addPoint(centerlinePosition + tubeOffset);
        }

        return pointCloud;
    }

}
