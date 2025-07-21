#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Point {

    std::string id;
    glm::vec3 position;
    //glm::vec3 color;  // next update

};

class PointCloudLoader {
public:
    static std::vector<Point> loadFromFile(const std::string& filePath);
};