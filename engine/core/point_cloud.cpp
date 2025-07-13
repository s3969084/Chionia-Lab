// src/point_cloud.cpp

#include "core/point_cloud.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace chionia {

    PointCloud::PointCloud(const std::vector<LabeledPoint>& points)
        : points_(points) {}

    void PointCloud::addPoint(const glm::vec4& position) {
        points_.emplace_back(position); // default label = 0
    }

    void PointCloud::translate(const glm::vec3& offset) {
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), offset);
        for (auto& point : points_) {
            point.position = translationMatrix * point.position;
        }
    }

    void PointCloud::scale(float factor) {
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(factor));
        for (auto& point : points_) {
            point.position = scaleMatrix * point.position;
        }
    }

    void PointCloud::rotate(float angleRadians, const glm::vec3& axis) {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleRadians, axis);
        for (auto& point : points_) {
            point.position = rotationMatrix * point.position;
        }
    }

    const std::vector<LabeledPoint>& PointCloud::getPoints() const {
        return points_;
    }

} // namespace chionia
