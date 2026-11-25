
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
#include <cmath>
#include <limits>
#include <iomanip>

#include "io/json_model_loader.hpp"
#include "scene/shapes/shape_resampler.hpp"
#include "scene/shapes/shape_asset.hpp"
#include "scene/shapes/shape_sampling_config.hpp"

namespace chionia {

    namespace {

        void printSpacingDiagnostics(
            const std::vector<glm::vec3>& positions
        ) {
            if (positions.size() < 2) {
                return;
            }

            double minimumDistance =
                std::numeric_limits<double>::max();

            double maximumDistance = 0.0;
            double totalDistance = 0.0;

            std::vector<double> distances;
            distances.reserve(positions.size() - 1);

            for (std::size_t i = 1; i < positions.size(); ++i) {
                const glm::vec3& a = positions[i - 1];
                const glm::vec3& b = positions[i];

                const double dx =
                    static_cast<double>(b.x) - a.x;

                const double dy =
                    static_cast<double>(b.y) - a.y;

                const double dz =
                    static_cast<double>(b.z) - a.z;

                const double distance =
                    std::hypot(dx, dy, dz);

                distances.push_back(distance);

                minimumDistance =
                    std::min(minimumDistance, distance);

                maximumDistance =
                    std::max(maximumDistance, distance);

                totalDistance += distance;
            }

            const double averageDistance =
                totalDistance /
                static_cast<double>(distances.size());

            double squaredDifferenceSum = 0.0;

            for (const double distance : distances) {
                const double difference =
                    distance - averageDistance;

                squaredDifferenceSum +=
                    difference * difference;
            }

            const double variance =
                squaredDifferenceSum /
                static_cast<double>(distances.size());

            const double standardDeviation =
                std::sqrt(variance);

            const auto oldFlags = std::cout.flags();
            const auto oldPrecision = std::cout.precision();

            std::cout
                << std::fixed
                << std::setprecision(8);

            std::cout
                << "[Shape Diagnostics] Consecutive spacing:"
                << "\n  Minimum: " << minimumDistance
                << "\n  Maximum: " << maximumDistance
                << "\n  Average: " << averageDistance
                << "\n  Std dev: " << standardDeviation
                << '\n';

            std::cout.flags(oldFlags);
            std::cout.precision(oldPrecision);
        }

        ShapeSamplingConfig getSamplingConfig(
            const std::string& filename
        ) {
            ShapeSamplingConfig config;

            if (
                filename.find("helix.json") != std::string::npos
            ) {
                config.mode = ShapeSamplingMode::Curve;
                config.curveCount = 1;
                config.closedCurve = false;
            }
            else if (
                filename.find("double.json") != std::string::npos
            ) {
                config.mode = ShapeSamplingMode::Curve;
                config.curveCount = 2;
                config.interleavedCurves = true;
                config.closedCurve = false;
            }
            else if (
                filename.find("trefoil_knot.json") != std::string::npos
            ) {
                config.mode = ShapeSamplingMode::Curve;
                config.curveCount = 1;
                config.closedCurve = true;

                // The source file contains two identical cycles.
                // Only the first 1,000 points are needed.
                config.sourcePointCount = 1000;
            }
            else if (
                filename.find("cube.json") != std::string::npos
            ) {
                config.mode = ShapeSamplingMode::Volume;

                // Confirmed source layout:
                // 11 points along X, Y and Z.
                config.uCount = 11;
                config.vCount = 11;
                config.wCount = 11;
            }
            else if (
                filename.find("sphere.json") != std::string::npos
            ) {
                config.mode = ShapeSamplingMode::Surface;

                // Confirmed source layout:
                // 73 longitude samples per row.
                // 37 latitude rows.
                config.uCount = 73;
                config.vCount = 37;

                // Longitude wraps around the sphere.
                // Latitude runs from north to south pole.
                config.wrapU = true;
                config.wrapV = false;
            }

            return config;
        }

    }

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

            auto queueShape = [this](const std::string& filename) {

                PointCloud sourceCloud =
                    PointCloudLoader::loadPointCloudFromFile(filename);

                const std::size_t sourcePointCount =
                    sourceCloud.getPointCount();

                const ShapeSamplingConfig samplingConfig =
                    getSamplingConfig(filename);

                std::vector<glm::vec3> targetPositions;

                if (
                    samplingConfig.mode == ShapeSamplingMode::Curve ||
                    samplingConfig.mode == ShapeSamplingMode::Volume ||
                    samplingConfig.mode == ShapeSamplingMode::Surface
                ) {
                    ShapeAsset asset(
                        std::move(sourceCloud),
                        samplingConfig
                    );

                    targetPositions =
                        ShapeResampler::resample(asset);
                }
                else {
                    // Temporary legacy path for shapes whose
                    // topology has not yet been configured.
                    targetPositions =
                        ShapeResampler::resample(sourceCloud);
                }

                if (
                    targetPositions.size() !=
                    ShapeResampler::PARTICLE_COUNT
                ) {
                    throw std::runtime_error(
                        "Shape resampler returned an incorrect particle count."
                    );
                }

                // Consecutive spacing is meaningful for a single
                // ordered curve, but not for a volume or surface.
                if (
                    samplingConfig.mode == ShapeSamplingMode::Curve &&
                    samplingConfig.curveCount == 1
                ) {
                    printSpacingDiagnostics(targetPositions);
                }

                PointCloud renderCloud(filename);

                for (const auto& position : targetPositions) {
                    renderCloud.addPoint(position);
                }

                std::cout
                    << "\n[Shape Loader] Loaded: " << filename
                    << "\n[Shape Loader] Source points: "
                    << sourcePointCount
                    << "\n[Shape Loader] Resampled points: "
                    << renderCloud.getPointCount()
                    << '\n';

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
