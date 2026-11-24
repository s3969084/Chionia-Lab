
#include "shape_resampler.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace chionia {

    namespace {

        std::vector<glm::vec3> resamplePositions(
            const std::vector<glm::vec3>& sourcePositions
        ) {
            if (sourcePositions.empty()) {
                throw std::invalid_argument(
                    "Cannot resample an empty point cloud."
                );
            }

            // Keep only points with valid coordinates.
            std::vector<glm::vec3> validPoints;
            validPoints.reserve(sourcePositions.size());

            for (const glm::vec3& position : sourcePositions) {

                if (
                    std::isfinite(position.x) &&
                    std::isfinite(position.y) &&
                    std::isfinite(position.z)
                ) {
                    validPoints.push_back(position);
                }
            }

            if (validPoints.empty()) {
                throw std::invalid_argument(
                    "Point cloud contains no valid coordinates."
                );
            }

            constexpr std::size_t targetCount =
                ShapeResampler::PARTICLE_COUNT;

            std::vector<glm::vec3> result;
            result.reserve(targetCount);

            // A single valid point is repeated to fill the population.
            if (validPoints.size() == 1) {
                result.assign(targetCount, validPoints.front());
                return result;
            }

            const std::size_t sourceCount = validPoints.size();

            // Resample the ordered source sequence to a fixed population.
            //
            // Each destination particle is interpolated between two
            // neighbouring source points. The first and last source
            // positions are retained.
            for (std::size_t i = 0; i < targetCount; ++i) {

                const double sourcePosition =
                    static_cast<double>(i) *
                    static_cast<double>(sourceCount - 1) /
                    static_cast<double>(targetCount - 1);

                const std::size_t indexA =
                    static_cast<std::size_t>(sourcePosition);

                const std::size_t indexB =
                    std::min(indexA + 1, sourceCount - 1);

                const float t = static_cast<float>(
                    sourcePosition - static_cast<double>(indexA)
                );

                const glm::vec3 position = glm::mix(
                    validPoints[indexA],
                    validPoints[indexB],
                    t
                );

                result.push_back(position);
            }

            return result;
        }

    }

    std::vector<glm::vec3> ShapeResampler::resample(
        const PointCloud& pointCloud
    ) {
        std::vector<glm::vec3> positions;
        positions.reserve(pointCloud.getPointCount());

        for (const PointCloudPoint& point : pointCloud.getPoints()) {
            positions.push_back(point.position);
        }

        return resamplePositions(positions);
    }

    std::vector<glm::vec3> ShapeResampler::resample(
        const std::vector<Point>& points
    ) {
        std::vector<glm::vec3> positions;
        positions.reserve(points.size());

        for (const Point& point : points) {
            positions.push_back(point.position);
        }

        return resamplePositions(positions);
    }

}
