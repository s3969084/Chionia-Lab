#pragma once

#include "io/json_model_loader.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstddef>

namespace chionia {

    class ShapeResampler {
    public:
        static constexpr std::size_t PARTICLE_COUNT = 12000;

        static std::vector<glm::vec3> resample(
            const std::vector<Point>& points
        );
    };

}