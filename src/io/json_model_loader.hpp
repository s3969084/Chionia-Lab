#pragma once

#include "../graphics/resources/point.hpp"
#include <vector>
#include <string>

namespace chionia {
    class PointCloudLoader {
    public:
        static std::vector<Point> loadFromFile(const std::string& filePath);
    };
}