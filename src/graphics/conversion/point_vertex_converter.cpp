#include "point_vertex_converter.hpp"

#include <stdexcept>

namespace chionia {

    std::vector<Vertex> PointVertexConverter::convertPointsToVertices(
        const std::vector<Point>& points
    ) {
        std::vector<Vertex> vertices;
        vertices.reserve(points.size());

        for (uint32_t i = 0; i < points.size(); ++i) {
            Vertex v;
            v.id = i;
            v.position = points[i].position;
            v.targetPosition = points[i].position;
            vertices.push_back(v);
        }

        return vertices;
    }

    std::vector<Vertex> PointVertexConverter::convertPositionsToVertices(
        const std::vector<glm::vec3>& startPositions,
        const std::vector<glm::vec3>& targetPositions
    ) {
        if (startPositions.size() != targetPositions.size()) {
            throw std::invalid_argument(
                "Starting and target positions must have equal sizes."
            );
        }

        std::vector<Vertex> vertices;
        vertices.reserve(startPositions.size());

        for (std::size_t i = 0; i < startPositions.size(); ++i) {
            Vertex vertex;

            vertex.id = static_cast<uint32_t>(i);
            vertex.position = startPositions[i];
            vertex.targetPosition = targetPositions[i];

            vertices.push_back(vertex);
        }

        return vertices;
    }

}