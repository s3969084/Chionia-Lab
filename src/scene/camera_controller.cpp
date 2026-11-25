#define GLM_ENABLE_EXPERIMENTAL
#include "camera_controller.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace chionia {

    // Static map to link GLFWwindow* to CameraController*
    std::unordered_map<GLFWwindow*, CameraController*> CameraController::controllerMap_;

    CameraController::CameraController(Camera& camera)
        : camera_(camera),
          orbitRadius_(2.4f),
          targetOrbitRadius_(2.4f),
          horizontalAngle_(glm::radians(180.0f)),
          verticalAngle_(0.0f),
          orbitTarget_(glm::vec3(0.0f)),
          dragging_(false),
          lastMousePos_(-1.0f, -1.0f) {}

    void CameraController::init(GLFWwindow* window) {
        bindScroll(window);
    }

    float CameraController::getTargetOrbitRadius() const {
        return targetOrbitRadius_;
    }

    void CameraController::setOrbitRadius(float radius) {
        targetOrbitRadius_ = glm::clamp(radius, 0.2f, 100.0f);
        orbitRadius_ = targetOrbitRadius_;
        applyCameraTransform();
    }

    glm::vec3 CameraController::getOrbitTarget() const {
        return orbitTarget_;
    }

    void CameraController::setOrbitTarget(const glm::vec3& target) {
        orbitTarget_ = target;
        applyCameraTransform();
    }

    void CameraController::shiftOrbitTargetHorizontally(
        float screenPixels,
        float viewportHeight
    ) {
        if (viewportHeight <= 0.0f || screenPixels == 0.0f) {
            return;
        }

        const float halfFieldOfView = glm::radians(
            camera_.getFieldOfView() * 0.5f
        );
        const float worldUnitsPerPixel =
            2.0f * orbitRadius_ * std::tan(halfFieldOfView) /
            viewportHeight;
        const glm::quat rotation =
            glm::angleAxis(horizontalAngle_, glm::vec3(0, 1, 0)) *
            glm::angleAxis(verticalAngle_, glm::vec3(1, 0, 0));
        const glm::vec3 screenRight =
            rotation * glm::vec3(1.0f, 0.0f, 0.0f);

        orbitTarget_ -= screenRight * screenPixels * worldUnitsPerPixel;
        applyCameraTransform();
    }

    void CameraController::setCinematicOrbit(bool enabled) {
        if (cinematicOrbitEnabled_ == enabled) {
            return;
        }

        cinematicOrbitEnabled_ = enabled;
        cinematicIdleSeconds_ = enabled ? 3.0f : 0.0f;
        cinematicBaseHorizontalAngle_ = horizontalAngle_;
        cinematicBaseVerticalAngle_ = verticalAngle_;
        cinematicPhase_ = 0.0f;
    }

    void CameraController::update(
        GLFWwindow* window,
        float deltaTime,
        bool allowUserInput
    ) {
        constexpr float CINEMATIC_RESUME_DELAY = 3.0f;
        constexpr float CINEMATIC_SWAY_AMPLITUDE = 0.13f;
        constexpr float CINEMATIC_SWAY_SPEED = 0.07f;
        constexpr float CINEMATIC_PITCH_AMPLITUDE = 0.015f;

        bool manualCameraInput = false;
        if (
            allowUserInput &&
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS
        ) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            glm::vec2 currentMousePos(xpos, ypos);

            if (!dragging_) {
                dragging_ = true;
                lastMousePos_ = currentMousePos;
            }
            else {
                glm::vec2 delta = currentMousePos - lastMousePos_;
                lastMousePos_ = currentMousePos;

                const float sensitivity = 0.005f;
                horizontalAngle_ -= delta.x * sensitivity;
                verticalAngle_ -= delta.y * sensitivity;

                const float pitchLimit = glm::radians(89.0f);
                verticalAngle_ = glm::clamp(
                    verticalAngle_,
                    -pitchLimit,
                    pitchLimit
                );
            }
            manualCameraInput = true;
        } else {
            dragging_ = false;
        }

        if (allowUserInput && scrollDelta_ != 0.0f) {
            const float zoomSpeed = 0.5f;
            targetOrbitRadius_ -= scrollDelta_ * zoomSpeed;
            targetOrbitRadius_ = glm::clamp(targetOrbitRadius_, 0.2f, 100.0f);
            manualCameraInput = true;
            scrollDelta_ = 0.0f;
        }
        else if (!allowUserInput) {
            scrollDelta_ = 0.0f;
        }

        if (cinematicOrbitEnabled_) {
            if (manualCameraInput) {
                cinematicIdleSeconds_ = 0.0f;
                cinematicBaseHorizontalAngle_ = horizontalAngle_;
                cinematicBaseVerticalAngle_ = verticalAngle_;
                cinematicPhase_ = 0.0f;
            }
            else {
                cinematicIdleSeconds_ += deltaTime;
                if (cinematicIdleSeconds_ >= CINEMATIC_RESUME_DELAY) {
                    cinematicPhase_ += CINEMATIC_SWAY_SPEED * deltaTime;
                    horizontalAngle_ = cinematicBaseHorizontalAngle_ +
                        std::sin(cinematicPhase_) * CINEMATIC_SWAY_AMPLITUDE;
                    verticalAngle_ = cinematicBaseVerticalAngle_ +
                        std::sin(cinematicPhase_ * 0.65f) *
                            CINEMATIC_PITCH_AMPLITUDE;
                }
            }
        }

        const float fullRotation = glm::two_pi<float>();
        horizontalAngle_ = std::fmod(horizontalAngle_, fullRotation);
        if (horizontalAngle_ < 0.0f) {
            horizontalAngle_ += fullRotation;
        }

        // Smooth zoom toward target
        const float zoomSmoothSpeed = 5.0f;
        orbitRadius_ += (targetOrbitRadius_ - orbitRadius_) * glm::clamp(deltaTime * zoomSmoothSpeed, 0.0f, 1.0f);

        // Apply camera transform
        applyCameraTransform();
    }

    void CameraController::applyCameraTransform() {
        const glm::quat rotation =
            glm::angleAxis(horizontalAngle_, glm::vec3(0, 1, 0)) *
            glm::angleAxis(verticalAngle_, glm::vec3(1, 0, 0));
        const glm::vec3 offset =
            rotation * glm::vec3(0, 0, orbitRadius_);
        camera_.getTransform().setPosition(orbitTarget_ + offset);
        camera_.getTransform().setRotation(glm::vec3(0.0f));
        camera_.setLookAtTarget(orbitTarget_);
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
