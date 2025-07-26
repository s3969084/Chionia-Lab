#pragma once

#include "utils/json_model_loader.hpp"
#include "engine/buffer_management/vertex.hpp"
#include <vector>

namespace chionia::utils {

    class PointVertexConverter {
    public:
        static std::vector<Vertex> convertPointsToVertices(const std::vector<Point>& points);
    };
}
