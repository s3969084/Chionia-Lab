#pragma once

#include "core/resources/point_cloud.hpp"
#include "scene/transform.hpp"

#include <cstdint>
#include <string>

namespace chionia {

    using SceneObjectId = std::uint64_t;

    class SceneObject {
    public:
        SceneObject(
            SceneObjectId id,
            std::string key,
            PointCloud pointCloud
        );

        SceneObjectId getId() const;
        const std::string& getKey() const;
        const PointCloud& getPointCloud() const;
        const Transform& getTransform() const;
        float getAnimationSpeed() const;

        void setPointCloud(PointCloud pointCloud);
        void setTransform(const Transform& transform);
        void setAnimationSpeed(float speed);

    private:
        SceneObjectId id_;
        std::string key_;
        PointCloud pointCloud_;
        Transform transform_;
        float animationSpeed_ = 1.0f;
    };

}
