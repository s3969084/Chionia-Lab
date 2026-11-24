
#pragma once

#include "../graphics/resources/point.hpp"
#include "../core/resources/point_cloud.hpp"

#include <string>
#include <vector>

namespace chionia {

    class PointCloudLoader {
    public:
        // Legacy loading path — retained for the existing application.
        static std::vector<Point> loadFromFile(
            const std::string& filePath
        );

        // New core-resource loading path.
        static PointCloud loadPointCloudFromFile(
            const std::string& filePath
        );
    };

}
