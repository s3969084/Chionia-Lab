#pragma once

#include "core/resources/point_cloud.hpp"
#include "scene/scene_object.hpp"
#include "scene/transform.hpp"
#include "scene/render_settings.hpp"

#include <string>
#include <variant>

namespace chionia {

    struct UpsertPointCloudCommand {
        std::string key;
        PointCloud pointCloud;
    };

    struct SetSceneObjectTransformCommand {
        SceneObjectId objectId = 0;
        Transform transform;
    };

    struct SetSceneObjectAnimationSpeedCommand {
        SceneObjectId objectId = 0;
        float speed = 1.0f;
    };

    struct SelectSceneObjectCommand {
        SceneObjectId objectId = 0;
    };

    struct SetSceneRenderSettingsCommand {
        SceneRenderSettings settings;
    };

    struct RemoveSceneObjectCommand {
        SceneObjectId objectId = 0;
    };

    using SceneCommand = std::variant<
        UpsertPointCloudCommand,
        SetSceneObjectTransformCommand,
        SetSceneObjectAnimationSpeedCommand,
        SelectSceneObjectCommand,
        SetSceneRenderSettingsCommand,
        RemoveSceneObjectCommand
    >;

}
