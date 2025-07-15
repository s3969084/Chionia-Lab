#include "application.hpp"
#include "utils/point_cloud_loader.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace chionia {

    Application::~Application() {
        terminal_.stop();  // Ensure the terminal thread is signaled to stop
    }

    void Application::run() {
        try {
            // Initialize Vulkan and terminal
            vk_.init();

            // Allow terminal to trigger point cloud loading
            terminal_.setLoadCallback([](const std::string& filename) {
                try {
                    auto points = PointCloudLoader::loadFromFile(filename);
                    std::cout << "✅ Loaded " << points.size() << " points from " << filename << "\n";
                    // TODO: forward to Vulkan (or other renderer) here
                } catch (const std::exception& e) {
                    std::cerr << "❌ Failed to load file: " << e.what() << "\n";
                }
            });

            terminal_.start();  // Start input thread

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

            // Cleanup Vulkan before waiting for terminal
            vk_.cleanup();
            terminal_.stop();

        } catch (const std::exception& e) {
            std::cerr << "❌ Exception: " << e.what() << std::endl;
        }

        std::cout << "✅ Clean shutdown complete.\n";
        std::exit(0);
    }
}
