# pragma once

#include <variant>
#include <vector>
#include <graphics/resources/vertex.hpp>

namespace chionia {

    enum class RenderCommandType {
        UpdateVertices,
        SetPointSize,
        ReloadPipeline,
        // future expansion of commands
    };

    struct RenderCommand {
        RenderCommandType type;
        std::variant<std::monostate, float, std::vector<Vertex>> data;
    };



}
