// engine/core/app_window.cpp

#include "app_window.hpp"
#include <iostream>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#endif

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

#if defined(_WIN32)
        HWND nativeWindow = glfwGetWin32Window(window_);
        BOOL useDarkTitleBar = TRUE;
        constexpr DWORD darkTitleBarAttribute = 20;
        if (FAILED(DwmSetWindowAttribute(
            nativeWindow,
            darkTitleBarAttribute,
            &useDarkTitleBar,
            sizeof(useDarkTitleBar)
        ))) {
            // Windows 10 builds before 20H1 used attribute 19.
            constexpr DWORD legacyDarkTitleBarAttribute = 19;
            DwmSetWindowAttribute(
                nativeWindow,
                legacyDarkTitleBarAttribute,
                &useDarkTitleBar,
                sizeof(useDarkTitleBar)
            );
        }
#endif

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
