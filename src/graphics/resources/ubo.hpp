
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
        // z = travelling energy effects enabled
        // w = elapsed time since orbital animation started
        glm::vec4 morph;

        // x = ground plane height used by the particle reflection
        // y = reflection enabled
        // z = object's lowest-point clearance above the ground
        // w = reserved
        glm::vec4 ground;
    };

    static_assert(
        std::is_standard_layout<UniformBufferObject>::value,
        "UniformBufferObject must be standard layout"
    );

    static_assert(
        sizeof(UniformBufferObject) == 240,
        "UniformBufferObject size does not match GLSL layout"
    );

}
