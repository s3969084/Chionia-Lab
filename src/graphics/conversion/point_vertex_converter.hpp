
#pragma once

#include "../resources/point.hpp"
#include "../../core/resources/point_cloud.hpp"
#include "../resources/vertex.hpp"

#include <vector>

namespace chionia {

    class PointVertexConverter {
    public:
        // Legacy conversion path.
        static std::vector<Vertex> convertPointsToVertices(
            const std::vector<Point>& points
        );

        // New PointCloud conversion path.
        static std::vector<Vertex> convertPointCloudToVertices(
            const PointCloud& pointCloud
        );

        // Existing shape-morphing conversion path.
        static std::vector<Vertex> convertPositionsToVertices(
            const std::vector<glm::vec3>& startPositions,
            const std::vector<glm::vec3>& targetPositions
        );
    };

}
