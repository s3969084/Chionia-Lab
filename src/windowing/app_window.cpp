// engine/core/app_window.cpp

#include "app_window.hpp"
#include <iostream>

namespace chionia {

    AppWindow::AppWindow(int width, int height, const std::string& title)
        : width_(width), height_(height), title_(title), window_(nullptr) {
        initGLFW();
        createWindow();
    }

    AppWindow::~AppWindow() {
        destroy();  // Ensure graceful shutdown on destruction
    }

    void AppWindow::initGLFW() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    void AppWindow::createWindow() {
        window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            std::exit(EXIT_FAILURE);
        }

        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);

        glfwShowWindow(window_);
    }

    void AppWindow::pollEvents() const {
        glfwPollEvents();
    }

    bool AppWindow::shouldClose() const {
        return glfwWindowShouldClose(window_);
    }

    GLFWwindow* AppWindow::getGLFWwindow() const {
        return window_;
    }

    void AppWindow::destroy() {
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
        std::cout << "GLFW window destroyed and GLFW terminated.\n";
    }

    void AppWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<AppWindow*>(glfwGetWindowUserPointer(window));
        if (app) app->markResized();
    }

}
