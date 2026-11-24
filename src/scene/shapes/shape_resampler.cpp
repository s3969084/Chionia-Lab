
#include "shape_resampler.hpp"

#include <cmath>
#include <stdexcept>

namespace chionia {

    std::vector<glm::vec3> ShapeResampler::resample(
        const std::vector<Point>& points
    ) {
        if (points.empty()) {
            throw std::invalid_argument(
                "Cannot resample an empty point cloud."
            );
        }

        // Remove invalid coordinates before sampling.
        std::vector<glm::vec3> validPoints;
        validPoints.reserve(points.size());

        for (const Point& point : points) {
            const glm::vec3& position = point.position;

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

        constexpr std::size_t targetCount = TARGET_PARTICLE_COUNT;

        std::vector<glm::vec3> result;
        result.reserve(targetCount);

        const std::size_t sourceCount = validPoints.size();

        for (std::size_t i = 0; i < targetCount; ++i) {

            // Evenly distribute samples across larger point clouds.
            // For smaller clouds, distribute repetitions evenly.
            const std::size_t sourceIndex =
                (i * sourceCount) / targetCount;

            result.push_back(validPoints[sourceIndex]);
        }

        return result;
    }

}
