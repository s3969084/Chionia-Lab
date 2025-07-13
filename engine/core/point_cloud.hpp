#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "utils/point_cloud_loader.hpp"

namespace chionia {

    class PointCloud {
    public:
        PointCloud() = default;
        explicit PointCloud(const std::vector<LabeledPoint>& points);

        // Add a single point to the cloud
        void addPoint(const glm::vec4& position);

        // Transform operations
        void translate(const glm::vec3& offset);
        void scale(float factor);
        void rotate(float angleRadians, const glm::vec3& axis);

        // Accessor
        const std::vector<LabeledPoint>& getPoints() const;

    private:
        std::vector<LabeledPoint> points_;
    };

} // namespace chionia
