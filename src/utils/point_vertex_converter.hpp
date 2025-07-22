#pragma once

#include "point_cloud_loader.hpp"
#include "engine/buffer_management/vertex_buffer.hpp"
#include <vector>

namespace chionia::utils {

    class PointVertexConverter {
    public:
        static std::vector<Vertex> convertPointsToVertices(const std::vector<Point>& points);
    };
}
