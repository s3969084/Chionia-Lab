
#include "point_vertex_converter.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace chionia {

    std::vector<Vertex> PointVertexConverter::convertPointsToVertices(
        const std::vector<Point>& points
    ) {
        std::vector<Vertex> vertices;
        vertices.reserve(points.size());

        for (std::size_t i = 0; i < points.size(); ++i) {
            Vertex vertex;

            vertex.id = static_cast<uint32_t>(i);
            vertex.position = points[i].position;
            vertex.targetPosition = points[i].position;

            vertices.push_back(vertex);
        }

        return vertices;
    }

    std::vector<Vertex>
    PointVertexConverter::convertPointCloudToVertices(
        const PointCloud& pointCloud
    ) {
        const auto& points = pointCloud.getPoints();

        if (
            points.size() >
            static_cast<std::size_t>(
                std::numeric_limits<uint32_t>::max()
            )
        ) {
            throw std::overflow_error(
                "PointCloud contains too many points for the "
                "current Vulkan Vertex index format."
            );
        }

        std::vector<Vertex> vertices;
        vertices.reserve(points.size());

        for (std::size_t i = 0; i < points.size(); ++i) {
            Vertex vertex;

            // Renderer-local index, not the PointCloud identity.
            vertex.id = static_cast<uint32_t>(i);

            vertex.position = points[i].position;
            vertex.targetPosition = points[i].position;
            vertex.color = points[i].color;
            vertex.targetColor = points[i].color;
            vertex.orbitMotion = points[i].orbitMotion;

            vertices.push_back(vertex);
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

        if (
            startPositions.size() >
            static_cast<std::size_t>(
                std::numeric_limits<uint32_t>::max()
            )
        ) {
            throw std::overflow_error(
                "Too many positions for the current Vulkan Vertex index format."
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
