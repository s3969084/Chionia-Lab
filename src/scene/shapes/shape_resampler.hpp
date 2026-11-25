
#pragma once

#include "core/resources/point_cloud.hpp"
#include "graphics/resources/point.hpp"
#include "shape_asset.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace chionia {

    class ShapeResampler {
    public:
        static constexpr std::size_t PARTICLE_COUNT = MORPH_PARTICLE_COUNT;

        // Geometry-aware interface.
        static std::vector<glm::vec3> resample(
            const ShapeAsset& asset
        );

        // Legacy PointCloud interface.
        static std::vector<glm::vec3> resample(
            const PointCloud& pointCloud
        );

        // Legacy interface retained for compatibility.
        static std::vector<glm::vec3> resample(
            const std::vector<Point>& points
        );
    };

}
