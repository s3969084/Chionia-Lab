#pragma once

#include <glm/glm.hpp>

namespace chionia {

    struct Vertex {
        uint32_t id;
        glm::vec3 position;

        Vertex() = default;

        Vertex(uint32_t id, const glm::vec3& pos)
            : id(id), position(pos) {}
    };
}