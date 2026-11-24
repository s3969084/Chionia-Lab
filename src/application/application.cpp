
#include "application.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <cstdlib>
#include <string>

#include "graphics/conversion/point_vertex_converter.hpp"
#include "io/json_model_loader.hpp"
#include "scene/shapes/shape_resampler.hpp"

namespace chionia {

    Application::Application()
        : vk_(800, 600, "Chionia Engine"),
          controller_(camera_) {
    }

    Application::~Application() {
        terminal_.stop();
    }

    void Application::run() {
        try {
            vk_.init();

            // All available shapes, relative to data/objects/.
            const std::vector<std::string> shapes = {
                "shape/cone.json",
                "shape/cube.json",
                "shape/double.json",
                "shape/helix.json",
                "shape/mobius.json",
                "shape/pyramid.json",
                "shape/sphere.json",
                "shape/superquadric.json",
                "shape/trefoil_knot.json"
            };

            // Random number generator.
            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_int_distribution<int> holdDistribution(3, 6);

            // Load a shape into the core PointCloud resource
            // and queue its resampled representation for Vulkan.
            auto queueShape = [this](const std::string& filename) {

                // Load JSON directly into the core resource.
                PointCloud pointCloud =
                    PointCloudLoader::loadPointCloudFromFile(filename);

                // Resample directly from PointCloud.
                auto targetPositions =
                    ShapeResampler::resample(pointCloud);

                // Verify the fixed population.
                if (
                    targetPositions.size() !=
                    ShapeResampler::PARTICLE_COUNT
                ) {
                    throw std::runtime_error(
                        "Shape resampler returned an incorrect particle count."
                    );
                }

                // Convert positions into GPU vertices.
                // This is currently the renderer boundary.
                auto vertices =
                    PointVertexConverter::convertPositionsToVertices(
                        targetPositions,
                        targetPositions
                    );

                std::cout
                    << "\n[Shape Loader] Loaded: " << filename
                    << "\n[Shape Loader] Core points: "
                    << pointCloud.getPointCount()
                    << "\n[Shape Loader] Resampled particles: "
                    << targetPositions.size()
                    << "\n[Shape Loader] GPU vertices: "
                    << vertices.size()
                    << '\n';

                // Queue the vertex update.
                RenderCommand cmd;
                cmd.type = RenderCommandType::UpdateVertices;
                cmd.data = std::move(vertices);

                vk_.getRenderQueue().enqueue(cmd);
            };

            // Manual terminal loading.
            terminal_.setLoadCallback(
                [this, &queueShape, &shapes](
                    const std::string& filename
                ) {
                    try {
                        queueShape(filename);

                        // Identify the manually loaded shape.
                        int matchedIndex = -1;

                        for (
                            std::size_t i = 0;
                            i < shapes.size();
                            ++i
                        ) {
                            const std::string expected =
                                "data/objects/" + shapes[i];

                            if (filename == expected) {
                                matchedIndex = static_cast<int>(i);
                                break;
                            }
                        }

                        manualShapeIndex_.store(matchedIndex);
                        manualLoadVersion_.fetch_add(1);

                    }
                    catch (const std::exception& e) {
                        std::cerr
                            << "[Shape Loader] Failed to load "
                            << filename
                            << ": "
                            << e.what()
                            << '\n';
                    }
                }
            );

            terminal_.start();

            // Create a shuffled sequence of shape indices.
            std::vector<int> shapeOrder(shapes.size());

            std::iota(
                shapeOrder.begin(),
                shapeOrder.end(),
                0
            );

            std::shuffle(
                shapeOrder.begin(),
                shapeOrder.end(),
                rng
            );

            std::size_t nextShapePosition = 0;
            int previousShapeIndex = -1;

            // Start with the first randomly selected shape.
            const int initialShapeIndex =
                shapeOrder[nextShapePosition++];

            queueShape(
                "data/objects/" + shapes[initialShapeIndex]
            );

            previousShapeIndex = initialShapeIndex;

            auto nextShapeTime =
                std::chrono::steady_clock::now()
                + std::chrono::seconds(holdDistribution(rng))
                + std::chrono::seconds(2);

            unsigned int observedManualVersion =
                manualLoadVersion_.load();

            bool controllerInitialized = false;

            // Main render loop.
            while (
                !terminal_.shouldExit() &&
                !vk_.getWindow().shouldClose()
            ) {

                glfwPollEvents();

                if (!controllerInitialized) {
                    controller_.init(
                        vk_.getWindow().getGLFWwindow()
                    );

                    controllerInitialized = true;
                }

                controller_.update(
                    vk_.getWindow().getGLFWwindow(),
                    0.016f
                );

                const auto now =
                    std::chrono::steady_clock::now();

                // Check whether the user manually loaded a shape.
                const unsigned int currentManualVersion =
                    manualLoadVersion_.load();

                if (
                    currentManualVersion !=
                    observedManualVersion
                ) {

                    observedManualVersion =
                        currentManualVersion;

                    const int manualIndex =
                        manualShapeIndex_.load();

                    if (manualIndex >= 0) {
                        previousShapeIndex = manualIndex;
                    }

                    // Reset automatic timer after manual loading.
                    nextShapeTime =
                        now
                        + std::chrono::seconds(holdDistribution(rng))
                        + std::chrono::seconds(2);

                    std::cout
                        << "[Auto Cycle] Manual load detected. "
                        << "Automatic timer reset.\n";
                }

                // Time to load the next automatic shape.
                else if (now >= nextShapeTime) {

                    // Start another shuffled cycle when exhausted.
                    if (
                        nextShapePosition >= shapeOrder.size()
                    ) {

                        std::shuffle(
                            shapeOrder.begin(),
                            shapeOrder.end(),
                            rng
                        );

                        nextShapePosition = 0;
                    }

                    int nextIndex =
                        shapeOrder[nextShapePosition++];

                    // Avoid immediately repeating the same shape.
                    if (
                        nextIndex == previousShapeIndex &&
                        shapeOrder.size() > 1
                    ) {

                        if (
                            nextShapePosition >= shapeOrder.size()
                        ) {

                            std::shuffle(
                                shapeOrder.begin(),
                                shapeOrder.end(),
                                rng
                            );

                            nextShapePosition = 0;
                        }

                        nextIndex =
                            shapeOrder[nextShapePosition++];
                    }

                    try {

                        queueShape(
                            "data/objects/" + shapes[nextIndex]
                        );

                        previousShapeIndex = nextIndex;

                        nextShapeTime =
                            now
                            + std::chrono::seconds(
                                holdDistribution(rng)
                            )
                            + std::chrono::seconds(2);

                    }
                    catch (const std::exception& e) {

                        std::cerr
                            << "[Auto Cycle] Failed to load shape: "
                            << e.what()
                            << '\n';

                        nextShapeTime =
                            now + std::chrono::seconds(3);
                    }
                }

                vk_.drawFrame(camera_);

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(16)
                );
            }

            if (vk_.getWindow().shouldClose()) {

                terminal_.requestExit();

                std::cin.setstate(
                    std::ios::eofbit
                );
            }

            terminal_.stop();
            vk_.cleanup();

        }
        catch (const std::exception& e) {

            std::cerr
                << "Exception: "
                << e.what()
                << std::endl;
        }

        std::cout << "Clean shutdown complete.\n";

        std::exit(0);
    }

} // namespace chionia
