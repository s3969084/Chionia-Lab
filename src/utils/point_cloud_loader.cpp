// src/utils/point_cloud_loader.cpp

#include "point_cloud_loader.hpp"
#include <fstream>
#include <iostream>

namespace chionia {

    std::vector<LabeledPoint> PointCloudLoader::loadFromFile(const std::string& filepath) {
        std::vector<LabeledPoint> points;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filepath << std::endl;
            return points;
        }

        nlohmann::json data;


        try {
            file >> data;
        } catch (const std::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            return points;
        }

        for (const auto& item : data) {
            if (item.contains("id") && item.contains("x") && item.contains("y") && item.contains("z")) {
                std::string id = item["id"];
                float x = item["x"];
                float y = item["y"];
                float z = item["z"];
                points.push_back({glm::vec4(x, y, z, 1.0f), id});
            } else {
                std::cerr << " skipping invalid point entry (missing id/x/y/z)" <<std::endl;
            }
        }

        std::cout << "Loaded " << points.size() << "labeled points from " << filepath << std::endl;
        return points;


    }
}