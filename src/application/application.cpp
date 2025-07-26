#include "application.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#include "engine/buffer_management/point_vertex_converter.hpp"

namespace chionia {

    Application::~Application() {
        terminal_.stop();  // Ensure the terminal thread is signaled to stop
    }

    void Application::run() {
        try {
            // Initialize Vulkan and terminal
            vk_.init();

            // Allow terminal to trigger point cloud loading
            terminal_.setLoadCallback([this](const std::string& filename) {
                try {
                    auto points = PointCloudLoader::loadFromFile(filename);
                    auto vertices = chionia::utils::PointVertexConverter::convertPointsToVertices(points);
                    std::cout << "✅ Loaded " << vertices.size() << " vertices from " << filename << "\n";
                    // Forward points to vulkan
                    vk_.updateVertices(vertices);

                } catch (const std::exception& e) {
                    std::cerr << "Failed to load file: " << e.what() << "\n";
                }
            });

            terminal_.start();  // Start an input thread

            // Main render loop
            while (!terminal_.shouldExit() && !vk_.getWindow().shouldClose()) {
                vk_.drawFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }

            // Handle forced window close (e.g., user clicks X)
            if (vk_.getWindow().shouldClose()) {
                terminal_.requestExit();               // Signal terminal thread to finish
                std::cin.setstate(std::ios::eofbit);   // Prevent blocking read
            }

            // Clean up Vulkan before waiting for the terminal
            vk_.cleanup();
            terminal_.stop();

        } catch (const std::exception& e) {
            std::cerr << "❌ Exception: " << e.what() << std::endl;
        }

        std::cout << "✅ Clean shutdown complete.\n";
        std::exit(0);
    }
}
