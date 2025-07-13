// engine/core/app_window.hpp

#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace chionia {

    class AppWindow {
    public:
        AppWindow(int width, int height, const std::string& title);
        ~AppWindow();

        bool shouldClose() const;
        void pollEvents() const;

        GLFWwindow* getGLFWwindow() const;

    private:
        void initGLFW();
        void createWindow();

        int width_;
        int height_;
        std::string title_;
        GLFWwindow* window_;




    };


}