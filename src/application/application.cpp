
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

            std::random_device rd;
            std::mt19937 rng(rd());
            std::uniform_int_distribution<int> holdDistribution(3, 6);

            // Load, resample and queue a core PointCloud.
            // No renderer-specific Vertex objects are created here.
            auto queueShape = [this](const std::string& filename) {

                PointCloud sourceCloud =
                    PointCloudLoader::loadPointCloudFromFile(filename);

                const auto targetPositions =
                    ShapeResampler::resample(sourceCloud);

                if (
                    targetPositions.size() !=
                    ShapeResampler::PARTICLE_COUNT
                ) {
                    throw std::runtime_error(
                        "Shape resampler returned an incorrect particle count."
                    );
                }

                // Build the resampled cloud as a core resource.
                PointCloud renderCloud(filename);

                for (const auto& position : targetPositions) {
                    renderCloud.addPoint(position);
                }

                std::cout
                    << "\n[Shape Loader] Loaded: " << filename
                    << "\n[Shape Loader] Source points: "
                    << sourceCloud.getPointCount()
                    << "\n[Shape Loader] Resampled points: "
                    << renderCloud.getPointCount()
                    << '\n';

                // Queue the core PointCloud.
                RenderCommand cmd;
                cmd.type = RenderCommandType::UpdatePointCloud;
                cmd.data = std::move(renderCloud);

                vk_.getRenderQueue().enqueue(std::move(cmd));
            };

            terminal_.setLoadCallback(
                [this, &queueShape, &shapes](
                    const std::string& filename
                ) {
                    try {
                        queueShape(filename);

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

                    nextShapeTime =
                        now
                        + std::chrono::seconds(holdDistribution(rng))
                        + std::chrono::seconds(2);

                    std::cout
                        << "[Auto Cycle] Manual load detected. "
                        << "Automatic timer reset.\n";
                }
                else if (now >= nextShapeTime) {

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
