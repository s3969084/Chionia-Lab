#include "scene.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace chionia {

    SceneObjectId Scene::upsertPointCloud(
        std::string key,
        PointCloud pointCloud
    ) {
        if (key.empty()) {
            throw std::invalid_argument(
                "Scene object key cannot be empty."
            );
        }

        const auto existingKey = objectIdsByKey_.find(key);
        if (existingKey != objectIdsByKey_.end()) {
            SceneObject* object = findObject(existingKey->second);
            if (object == nullptr) {
                throw std::logic_error(
                    "Scene object key refers to a missing object."
                );
            }

            object->setPointCloud(std::move(pointCloud));
            return object->getId();
        }

        if (nextObjectId_ == 0) {
            throw std::overflow_error(
                "Scene object ID space exhausted."
            );
        }

        const SceneObjectId objectId = nextObjectId_++;
        SceneObject object(objectId, key, std::move(pointCloud));
        objects_.emplace(objectId, std::move(object));
        objectIdsByKey_.emplace(std::move(key), objectId);

        return objectId;
    }

    bool Scene::setTransform(
        SceneObjectId objectId,
        const Transform& transform
    ) {
        SceneObject* object = findObject(objectId);
        if (object == nullptr) {
            return false;
        }

        object->setTransform(transform);
        return true;
    }

    bool Scene::setAnimationSpeed(SceneObjectId objectId, float speed) {
        if (!std::isfinite(speed) || speed < 0.0f) {
            throw std::invalid_argument(
                "Scene object animation speed must be finite and non-negative."
            );
        }

        SceneObject* object = findObject(objectId);
        if (object == nullptr) {
            return false;
        }

        object->setAnimationSpeed(speed);
        return true;
    }

    const SceneRenderSettings& Scene::getRenderSettings() const {
        return renderSettings_;
    }

    void Scene::setRenderSettings(const SceneRenderSettings& settings) {
        if (
            !std::isfinite(settings.morphDurationSeconds) ||
            settings.morphDurationSeconds <= 0.0f ||
            !std::isfinite(settings.vibrationAmplitude) ||
            settings.vibrationAmplitude < 0.0f ||
            !std::isfinite(settings.vibrationSpeed) ||
            settings.vibrationSpeed < 0.0f
        ) {
            throw std::invalid_argument(
                "Scene render settings contain an invalid numeric value."
            );
        }

        renderSettings_ = settings;
    }

    bool Scene::removeObject(SceneObjectId objectId) {
        const auto object = objects_.find(objectId);
        if (object == objects_.end()) {
            return false;
        }

        objectIdsByKey_.erase(object->second.getKey());
        objects_.erase(object);
        return true;
    }

    SceneObject* Scene::findObject(SceneObjectId objectId) {
        const auto object = objects_.find(objectId);
        return object == objects_.end() ? nullptr : &object->second;
    }

    const SceneObject* Scene::findObject(SceneObjectId objectId) const {
        const auto object = objects_.find(objectId);
        return object == objects_.end() ? nullptr : &object->second;
    }

    std::optional<SceneObjectId> Scene::findObjectByKey(
        std::string_view key
    ) const {
        const auto object = objectIdsByKey_.find(std::string(key));
        if (object == objectIdsByKey_.end()) {
            return std::nullopt;
        }

        return object->second;
    }

    std::vector<SceneObjectId> Scene::getObjectIds() const {
        std::vector<SceneObjectId> objectIds;
        objectIds.reserve(objects_.size());

        for (const auto& [objectId, object] : objects_) {
            (void)object;
            objectIds.push_back(objectId);
        }

        std::sort(objectIds.begin(), objectIds.end());
        return objectIds;
    }

    std::size_t Scene::getObjectCount() const {
        return objects_.size();
    }

}
