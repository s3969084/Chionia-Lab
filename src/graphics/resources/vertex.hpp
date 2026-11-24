
#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace chionia {

    struct Vertex {
        uint32_t id = 0;
        glm::vec3 position{0.0f};
        glm::vec3 targetPosition{0.0f};

        Vertex() = default;

        Vertex(uint32_t id, const glm::vec3& pos)
            : id(id),
              position(pos),
              targetPosition(pos) {}
    };

}
