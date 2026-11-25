
#pragma once

#include "core/resources/point_cloud.hpp"

#include <cstdint>
#include <glm/glm.hpp>

namespace chionia {

    struct Vertex {
        uint32_t id = 0;
        glm::vec3 position{0.0f};
        glm::vec3 targetPosition{0.0f};
        glm::vec4 color{1.0f};
        PointOrbitMotion orbitMotion;

        Vertex() = default;

        Vertex(uint32_t id, const glm::vec3& pos)
            : id(id),
              position(pos),
              targetPosition(pos) {}
    };

}
