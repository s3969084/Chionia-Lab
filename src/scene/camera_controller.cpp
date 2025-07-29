#define GLM_ENABLE_EXPERIMENTAL
#include "camera_controller.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace chionia {

    // Static map to link GLFWwindow* to CameraController*
    std::unordered_map<GLFWwindow*, CameraController*> CameraController::controllerMap_;

    CameraController::CameraController(Camera& camera)
        : camera_(camera),
          orbitRadius_(2.5f),
          targetOrbitRadius_(2.5f),
          horizontalAngle_(glm::radians(180.0f)),
          verticalAngle_(0.0f),
          orbitTarget_(glm::vec3(0.0f)),
          dragging_(false),
          lastMousePos_(-1.0f, -1.0f) {}

    void CameraController::init(GLFWwindow* window) {
        bindScroll(window);
    }

    void CameraController::update(GLFWwindow* window, float deltaTime) {
        // Handle mouse drag
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            glm::vec2 currentMousePos(xpos, ypos);

            if (!dragging_) {
                dragging_ = true;
                lastMousePos_ = currentMousePos;
                return;
            }

            glm::vec2 delta = currentMousePos - lastMousePos_;
            lastMousePos_ = currentMousePos;

            const float sensitivity = 0.005f;
            horizontalAngle_ -= delta.x * sensitivity;
            verticalAngle_   -= delta.y * sensitivity;

            float pitchLimit = glm::radians(89.0f);
            verticalAngle_ = glm::clamp(verticalAngle_, -pitchLimit, pitchLimit);

            if (horizontalAngle_ > glm::two_pi<float>())
                horizontalAngle_ -= glm::two_pi<float>();
            else if (horizontalAngle_ < 0.0f)
                horizontalAngle_ += glm::two_pi<float>();
        } else {
            dragging_ = false;
        }

        // Apply scroll input to zoom target
        if (scrollDelta_ != 0.0f) {
            const float zoomSpeed = 0.5f;
            targetOrbitRadius_ -= scrollDelta_ * zoomSpeed;
            targetOrbitRadius_ = glm::clamp(targetOrbitRadius_, 0.2f, 100.0f);
            scrollDelta_ = 0.0f;
        }

        // Smooth zoom toward target
        const float zoomSmoothSpeed = 5.0f;
        orbitRadius_ += (targetOrbitRadius_ - orbitRadius_) * glm::clamp(deltaTime * zoomSmoothSpeed, 0.0f, 1.0f);

        // Apply camera transform
        glm::quat rotation = glm::angleAxis(horizontalAngle_, glm::vec3(0, 1, 0)) *
                             glm::angleAxis(verticalAngle_, glm::vec3(1, 0, 0));
        glm::vec3 offset = rotation * glm::vec3(0, 0, orbitRadius_);
        glm::vec3 position = orbitTarget_ + offset;

        camera_.getTransform().setPosition(position);
        camera_.getTransform().setRotation(glm::vec3(0.0f));  // Prevent roll
    }

    void CameraController::scrollCallback(GLFWwindow* window, double, double yoffset) {
        auto it = controllerMap_.find(window);
        if (it != controllerMap_.end()) {
            it->second->scrollDelta_ += static_cast<float>(yoffset);
        }
    }

    void CameraController::bindScroll(GLFWwindow* window) {
        controllerMap_[window] = this;
        glfwSetScrollCallback(window, scrollCallback);
    }

} // namespace chionia
