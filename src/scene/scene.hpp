#pragma once

#include "scene_object.hpp"
#include "render_settings.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace chionia {

    class Scene {
    public:
        // Replaces the point cloud for an existing key, preserving its ID
        // and transform, or creates a new object for an unseen key.
        SceneObjectId upsertPointCloud(
            std::string key,
            PointCloud pointCloud
        );

        bool setTransform(
            SceneObjectId objectId,
            const Transform& transform
        );

        bool setAnimationSpeed(SceneObjectId objectId, float speed);

        const SceneRenderSettings& getRenderSettings() const;
        void setRenderSettings(const SceneRenderSettings& settings);

        bool removeObject(SceneObjectId objectId);

        SceneObject* findObject(SceneObjectId objectId);
        const SceneObject* findObject(SceneObjectId objectId) const;

        std::optional<SceneObjectId> findObjectByKey(
            std::string_view key
        ) const;

        std::vector<SceneObjectId> getObjectIds() const;
        std::size_t getObjectCount() const;

    private:
        SceneObjectId nextObjectId_ = 1;
        std::unordered_map<SceneObjectId, SceneObject> objects_;
        std::unordered_map<std::string, SceneObjectId> objectIdsByKey_;
        SceneRenderSettings renderSettings_;
    };

}
