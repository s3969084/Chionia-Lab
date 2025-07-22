// src/engine/transform/camera.hpp

#pragma once

#include "transform.hpp"
#include <glm/glm.hpp>

namespace chionia {

    class Camera {
    public:
        Camera();

        void setPerspective(float fov, float near, float far);

        Transform& getTransform();
        const Transform& getTransform() const;

        glm::mat4 getViewMatrix() const;
        glm::mat4 getProjectionMatrix(float aspectRatio, bool flipY = false) const;

    private:
        Transform transform_;

        float fov_;
        float near_;
        float far_;

    };
}
