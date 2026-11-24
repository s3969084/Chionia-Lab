
#pragma once

#include "core/resources/point_cloud.hpp"
#include "graphics/resources/vertex.hpp"

#include <variant>
#include <vector>

namespace chionia {

    enum class RenderCommandType {
        UpdateVertices,
        UpdatePointCloud,
        SetPointSize,
        ReloadPipeline
    };

    struct RenderCommand {

        RenderCommandType type;

        std::variant<
            std::monostate,
            float,
            std::vector<Vertex>,
            PointCloud
        > data;
    };

}
