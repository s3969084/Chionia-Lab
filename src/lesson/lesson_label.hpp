#pragma once

#include <glm/glm.hpp>

#include <string_view>

namespace chionia {

    struct LessonLabel {
        std::string_view text;
        glm::vec3 position{0.0f};
        glm::vec4 color{1.0f};
    };

}
