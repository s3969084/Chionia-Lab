#pragma once

#include <string>
#include <GLFW/glfw3.h>

namespace chionia {

    class AppWindow {
    public:
        AppWindow(int width, int height, const std::string& title);
        ~AppWindow();

        GLFWwindow* getGLFWwindow() const;
        bool shouldClose() const;
        void pollEvents() const;
        void destroy();  // Explicit manual destruction

        // Window Resizing
        void markResized() { resized_ = true; }
        bool wasResized() const { return resized_;}
        void resetResizeFlag() { resized_ = false; }



    private:
        void initGLFW();
        void createWindow();

        int width_;
        int height_;
        std::string title_;
        GLFWwindow* window_;

        bool resized_ = false;

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    };

}
