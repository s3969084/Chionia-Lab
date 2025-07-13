#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Point {

    glm::vec3 position;
    glm::vec3 color;

};

class PointCloudLoader {
public:
    static std::vector<Point> loadFromFile(const std::string& filePath);
};