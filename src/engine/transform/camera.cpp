#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace chionia {

    Camera::Camera()
        : fov_(45.0f), near_(0.1f), far_(10.0f) {
        transform_.setPosition(glm::vec3(0.0f, 0.0f, 2.5f));
        transform_.setRotation(glm::vec3(0.0f));
        transform_.setScale(glm::vec3(1.0f));
    }

    void Camera::setPerspective(float fov, float near, float far) {
        fov_ = fov;
        near_ = near;
        far_ = far;
    }

    Transform& Camera::getTransform() {
        return transform_;
    }

    const Transform& Camera::getTransform() const {
        return transform_;
    }

    glm::mat4 Camera::getViewMatrix() const {
        return glm::lookAt(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(0,0,0), glm::vec3(0,1,0));
    }

    glm::mat4 Camera::getProjectionMatrix(float aspectRatio, bool flipY) const {
        glm::mat4 proj = glm::perspective(glm::radians(fov_), aspectRatio, near_, far_);
        if (flipY) proj[1][1] *= -1.0f;
        return proj;
    }




}