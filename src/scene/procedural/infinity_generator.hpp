#pragma once

#include "core/resources/point_cloud.hpp"

#include <cstddef>

namespace chionia {

    struct InfinityGeneratorConfig {
        std::size_t pointCount = 12000;

        // Radius of the point distribution around the curve's centerline.
        float thickness = 0.025f;
    };

    class InfinityGenerator {
    public:
        static PointCloud generate(
            const InfinityGeneratorConfig& config = {}
        );
    };

}
