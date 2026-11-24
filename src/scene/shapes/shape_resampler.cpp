#include "shape_resampler.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace chionia {

    std::vector<glm::vec3> ShapeResampler::resample(
        const std::vector<Point>& points
    ) {
        if (points.empty()) {
            throw std::invalid_argument(
                "Cannot resample an empty point cloud."
            );
        }

        // Keep only points with valid coordinates.
        std::vector<glm::vec3> validPoints;
        validPoints.reserve(points.size());

        for (const Point& point : points) {
            const glm::vec3& p = point.position;

            if (
                std::isfinite(p.x) &&
                std::isfinite(p.y) &&
                std::isfinite(p.z)
            ) {
                validPoints.push_back(p);
            }
        }

        if (validPoints.empty()) {
            throw std::invalid_argument(
                "Point cloud contains no valid coordinates."
            );
        }

        constexpr std::size_t targetCount = PARTICLE_COUNT;

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