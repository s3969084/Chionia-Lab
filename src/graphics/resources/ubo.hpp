
#pragma once

#include <glm/glm.hpp>
#include <type_traits>

namespace chionia {

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;

        // x = elapsed time
        // y = vibration amplitude
        // z = vibration speed
        // w = vibration enabled
        glm::vec4 animation;

        // x = morph progress (0.0 to 1.0)
        // y = morph enabled
        // z/w = reserved
        glm::vec4 morph;
    };

    static_assert(
        std::is_standard_layout<UniformBufferObject>::value,
        "UniformBufferObject must be standard layout"
    );

    static_assert(
        sizeof(UniformBufferObject) == 224,
        "UniformBufferObject size does not match GLSL layout"
    );

}
