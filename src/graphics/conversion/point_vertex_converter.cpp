#include "point_vertex_converter.hpp"

namespace chionia {

    std::vector<Vertex> PointVertexConverter::convertPointsToVertices(const std::vector<Point>& points) {
        std::vector<Vertex> vertices;
        vertices.reserve(points.size());

        for (uint32_t i = 0; i < points.size(); ++i) {
            Vertex v;
            v.id = i;
            v.position = points[i].position;
            vertices.push_back(v);
        }

        return vertices;
    }

}