#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <unordered_map>

#include "scene/camera.hpp"

namespace chionia {

    class CameraController {
    public:
        explicit CameraController(Camera& camera);
        void init(GLFWwindow* window);
        void update(
            GLFWwindow* window,
            float deltaTime,
            bool allowUserInput = true
        );

        float getTargetOrbitRadius() const;
        void setOrbitRadius(float radius);
        glm::vec3 getOrbitTarget() const;
        void setOrbitTarget(const glm::vec3& target);
        glm::vec2 getOrbitAngles() const;
        void setOrbitAngles(const glm::vec2& angles);
        void shiftOrbitTargetHorizontally(float screenPixels, float viewportHeight);
        void setCinematicOrbit(bool enabled);

    private:
        static void scrollCallback(GLFWwindow* window, double, double yoffset);
        static std::unordered_map<GLFWwindow*, CameraController*> controllerMap_;

        void bindScroll(GLFWwindow* window);
        void applyCameraTransform();

        Camera& camera_;
        glm::vec3 orbitTarget_;
        float orbitRadius_;
        float targetOrbitRadius_;
        float horizontalAngle_;
        float verticalAngle_;
        bool dragging_;
        glm::vec2 lastMousePos_;
        float scrollDelta_ = 0.0f;
        bool cinematicOrbitEnabled_ = false;
        float cinematicIdleSeconds_ = 0.0f;
        float cinematicBaseHorizontalAngle_ = 0.0f;
        float cinematicBaseVerticalAngle_ = 0.0f;
        float cinematicPhase_ = 0.0f;
    };

}
