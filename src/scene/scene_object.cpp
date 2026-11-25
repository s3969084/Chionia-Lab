#include "scene_object.hpp"

#include <utility>

namespace chionia {

    SceneObject::SceneObject(
        SceneObjectId id,
        std::string key,
        PointCloud pointCloud
    )
        : id_(id),
          key_(std::move(key)),
          pointCloud_(std::move(pointCloud)) {
    }

    SceneObjectId SceneObject::getId() const {
        return id_;
    }

    const std::string& SceneObject::getKey() const {
        return key_;
    }

    const PointCloud& SceneObject::getPointCloud() const {
        return pointCloud_;
    }

    const Transform& SceneObject::getTransform() const {
        return transform_;
    }

    float SceneObject::getAnimationSpeed() const {
        return animationSpeed_;
    }

    void SceneObject::setPointCloud(PointCloud pointCloud) {
        pointCloud_ = std::move(pointCloud);
    }

    void SceneObject::setTransform(const Transform& transform) {
        transform_ = transform;
    }

    void SceneObject::setAnimationSpeed(float speed) {
        animationSpeed_ = speed;
    }

}
