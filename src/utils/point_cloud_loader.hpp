// src/utils/point_cloud_loader.hpp
#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>



namespace chionia {

    struct LabeledPoint {
        glm::vec4 position;  // x, y, z, w
        std::string id;

        LabeledPoint(const glm::vec4& pos, const std::string& lbl = "")
            : position(pos), id(lbl) {}

    };


    class PointCloudLoader {
    public:
        // Loads points from a JSON file and returns them as a vector of glm::vec3
        static std::vector<LabeledPoint> loadFromFile(const std::string& filepath);
    };
}



