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
        void update(GLFWwindow* window, float deltaTime);

    private:
        static void scrollCallback(GLFWwindow* window, double, double yoffset);
        static std::unordered_map<GLFWwindow*, CameraController*> controllerMap_;

        void bindScroll(GLFWwindow* window);

        Camera& camera_;
        glm::vec3 orbitTarget_;
        float orbitRadius_;
        float targetOrbitRadius_;
        float horizontalAngle_;
        float verticalAngle_;
        bool dragging_;
        glm::vec2 lastMousePos_;
        float scrollDelta_ = 0.0f;
    };

}
