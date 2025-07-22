// src/engine/buffer_management/vertex.cpp

#pragma once

#include <glm/glm.hpp>

namespace chionia {

    struct Vertex {
        uint32_t id;
        glm::vec3 position;
    };
}