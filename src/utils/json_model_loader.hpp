#pragma once

#include "engine/buffer_management/point.hpp"
#include <vector>
#include <string>

namespace chionia {
    class PointCloudLoader {
    public:
        static std::vector<Point> loadFromFile(const std::string& filePath);
    };
}