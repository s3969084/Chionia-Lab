#include "application.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#include "graphics/conversion/point_vertex_converter.hpp"

namespace chionia {

    Application::Application() : vk_(800, 600, "Chionia Engine"), controller_(camera_) {}

    Application::~Application() {
        terminal_.stop();  // Ensure the terminal thread is signaled to stop
    }

    void Application::run() {
        try {
            // Initialize Vulkan (creates GLFW window)
            vk_.init();

            // Set terminal load callback
            terminal_.setLoadCallback([this](const std::string& filename) {
                try {
                    auto points = PointCloudLoader::loadFromFile(filename);
                    auto vertices = PointVertexConverter::convertPointsToVertices(points);
                    std::cout << "Loaded point cloud: " << vertices.size() << " vertices from " << filename << "\n";
                    vk_.updateVertices(vertices);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load file: " << e.what() << "\n";
                }
            });

            // Start terminal input thread
            terminal_.start();

            bool controllerInitialized = false;

            // Main render loop
            while (!terminal_.shouldExit() && !vk_.getWindow().shouldClose()) {
                glfwPollEvents();

                // Initialize controller inside main thread loop (safe)
                if (!controllerInitialized) {
                    controller_.init(vk_.getWindow().getGLFWwindow());
                    controllerInitialized = true;
                }

                controller_.update(vk_.getWindow().getGLFWwindow(), 0.016f);
                vk_.drawFrame(camera_);
                std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
            }

            // Graceful shutdown if window closed
            if (vk_.getWindow().shouldClose()) {
                terminal_.requestExit();
                std::cin.setstate(std::ios::eofbit);  // Prevent blocking read
            }

            // Cleanup
            vk_.cleanup();
            terminal_.stop();

        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }

        std::cout << "Clean shutdown complete.\n";
        std::exit(0);
    }

} // namespace chionia