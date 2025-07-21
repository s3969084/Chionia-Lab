#pragma once

#include "point_cloud_loader.hpp"
#include "vulkan_backend/vulkan_vertex_buffer.hpp"
#include <vector>

namespace chionia::utils {

    class PointVertexConverter {
    public:
        static std::vector<Vertex> convertPointsToVertices(const std::vector<Point>& points);
    };
}
