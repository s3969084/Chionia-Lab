#pragma once

#include <glm/glm.hpp>
#include <type_traits>

namespace chionia {
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 animation;
    };

    static_assert(std::is_standard_layout<UniformBufferObject>::value,
    "UniformBufferObject must be standard layout");

    static_assert(sizeof(UniformBufferObject) == 208,
        "UniformBufferObject size does not match GLSL layout");
}