#pragma once

#include "../../io/json_model_loader.hpp"
#include "../resources/vertex.hpp"
#include <vector>

namespace chionia {

    class PointVertexConverter {
    public:
        static std::vector<Vertex> convertPointsToVertices(const std::vector<Point>& points);
    };
}
