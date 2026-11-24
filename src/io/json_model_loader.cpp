
#include "json_model_loader.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace chionia {

    std::vector<Point> PointCloudLoader::loadFromFile(
        const std::string& filePath
    ) {
        std::vector<Point> points;
        std::ifstream file(filePath);

        if (!file.is_open()) {
            throw std::runtime_error(
                "Failed to open point cloud file: " + filePath
            );
        }

        json j;
        file >> j;

        for (const auto& entry : j) {
            std::string id = entry.at("id").get<std::string>();

            float x = entry.at("x").get<float>();
            float y = entry.at("y").get<float>();
            float z = entry.at("z").get<float>();

            Point point;
            point.id = id;
            point.position = glm::vec3(x, y, z);

            points.push_back(point);
        }

        return points;
    }

    PointCloud PointCloudLoader::loadPointCloudFromFile(
        const std::string& filePath
    ) {
        const std::vector<Point> sourcePoints =
            loadFromFile(filePath);

        PointCloud pointCloud(filePath);

        for (const Point& sourcePoint : sourcePoints) {
            pointCloud.addPoint(
                sourcePoint.position,
                glm::vec4(1.0f),
                1.0f,
                sourcePoint.id
            );
        }

        return pointCloud;
    }

}
