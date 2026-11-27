
#pragma once

#include "core/resources/point_cloud.hpp"
#include "graphics/resources/vertex.hpp"

#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace chionia {

    enum class RenderCommandType {
        UpdateVertices,
        UpdatePointCloud,
        UpdatePointCloudInstant,
        SetModelMatrix,
        SetAnimationSpeed,
        SetMorphDuration,
        SetVibrationAmplitude,
        SetVibrationSpeed,
        SetVibrationEnabled,
        SetMorphingEnabled,
        SetGroundReflectionEnabled,
        SetEnergyEffectsEnabled,
        SetPointSize,
        ReloadPipeline
    };

    struct RenderCommand {

        RenderCommandType type;

        std::variant<
            std::monostate,
            float,
            std::vector<Vertex>,
            PointCloud,
            glm::mat4,
            bool
        > data;
    };

}
