#pragma once

#include "../../io/json_model_loader.hpp"
#include "../resources/vertex.hpp"

#include <vector>

namespace chionia {

    class PointVertexConverter {
    public:
        static std::vector<Vertex> convertPointsToVertices(
            const std::vector<Point>& points
        );

        static std::vector<Vertex> convertPositionsToVertices(
            const std::vector<glm::vec3>& startPositions,
            const std::vector<glm::vec3>& targetPositions
        );
    };

}