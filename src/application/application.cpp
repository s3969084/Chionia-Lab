
#include "application.hpp"

#include <iostream>
#include <array>
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
#include <string_view>
#include <imgui.h>

#include "io/json_model_loader.hpp"
#include "scene/shapes/shape_resampler.hpp"
#include "scene/shapes/shape_asset.hpp"
#include "scene/shapes/shape_sampling_config.hpp"
#include "core/commands/scene_command.hpp"

namespace chionia {

    namespace {

        constexpr std::array<std::string_view, 9> AUTO_MORPH_SHAPES = {{
            "data/objects/shape/mobius.json",
            "data/objects/shape/superquadric.json",
            "data/objects/shape/double.json",
            "data/objects/shape/cone.json",
            "data/objects/shape/helix.json",
            "data/objects/shape/cube.json",
            "data/objects/shape/pyramid.json",
            "data/objects/shape/sphere.json",
            "data/objects/shape/trefoil_knot.json"
        }};

        constexpr auto AUTO_MORPH_INTERVAL = std::chrono::seconds(5);

        std::optional<std::size_t> findAutoMorphShapeIndex(
            std::string_view key
        ) {
            for (std::size_t i = 0; i < AUTO_MORPH_SHAPES.size(); ++i) {
                if (AUTO_MORPH_SHAPES[i] == key) {
                    return i;
                }
            }
            return std::nullopt;
        }

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
        : vk_(1600, 900, "Chionia Lab"),
          controller_(camera_) {
    }

    Application::~Application() {
        terminal_.stop();
    }

    void Application::processSceneCommands() {
        bool activeObjectChanged = false;
        bool activeTransformChanged = false;

        while (auto command = sceneCommandQueue_.tryDequeue()) {
            if (auto* upsert = std::get_if<UpsertPointCloudCommand>(&*command)) {
                const auto autoMorphIndex =
                    findAutoMorphShapeIndex(upsert->key);
                activeSceneObjectId_ = scene_.upsertPointCloud(
                    std::move(upsert->key),
                    std::move(upsert->pointCloud)
                );
                activeObjectChanged = true;

                if (autoMorphIndex) {
                    autoMorphShapeIndex_ = *autoMorphIndex;
                    hasAutoMorphShape_ = true;
                    nextAutoMorphTime_ =
                        std::chrono::steady_clock::now() +
                        AUTO_MORPH_INTERVAL;
                }
            }
            else if (
                auto* setTransform =
                    std::get_if<SetSceneObjectTransformCommand>(&*command)
            ) {
                if (
                    scene_.setTransform(
                        setTransform->objectId,
                        setTransform->transform
                    ) &&
                    activeSceneObjectId_ == setTransform->objectId
                ) {
                    activeTransformChanged = true;
                }
            }
            else if (
                auto* setAnimationSpeed =
                    std::get_if<SetSceneObjectAnimationSpeedCommand>(&*command)
            ) {
                if (
                    scene_.setAnimationSpeed(
                        setAnimationSpeed->objectId,
                        setAnimationSpeed->speed
                    ) &&
                    activeSceneObjectId_ == setAnimationSpeed->objectId
                ) {
                    activeObjectChanged = true;
                }
            }
            else if (
                auto* selectObject =
                    std::get_if<SelectSceneObjectCommand>(&*command)
            ) {
                if (scene_.findObject(selectObject->objectId) != nullptr) {
                    activeSceneObjectId_ = selectObject->objectId;
                    activeObjectChanged = true;
                }
            }
            else if (
                auto* setRenderSettings =
                    std::get_if<SetSceneRenderSettingsCommand>(&*command)
            ) {
                const bool wasAutoMorphEnabled = autoMorphEnabled_;
                scene_.setRenderSettings(setRenderSettings->settings);
                autoMorphEnabled_ =
                    scene_.getRenderSettings().autoMorphEnabled;
                if (!autoMorphEnabled_) {
                    nextAutoMorphTime_ = {};
                }
                else if (!wasAutoMorphEnabled && hasAutoMorphShape_) {
                    nextAutoMorphTime_ =
                        std::chrono::steady_clock::now() +
                        AUTO_MORPH_INTERVAL;
                }
                queueRenderSettingsForRendering(
                    scene_.getRenderSettings()
                );
            }
            else if (
                auto* removeObject =
                    std::get_if<RemoveSceneObjectCommand>(&*command)
            ) {
                const bool wasActive =
                    activeSceneObjectId_ == removeObject->objectId;

                if (scene_.removeObject(removeObject->objectId) && wasActive) {
                    const std::vector<SceneObjectId> objectIds =
                        scene_.getObjectIds();

                    if (objectIds.empty()) {
                        activeSceneObjectId_.reset();
                    }
                    else {
                        activeSceneObjectId_ = objectIds.back();
                    }

                    activeObjectChanged = true;
                }
            }
        }

        if (activeObjectChanged) {
            queueActiveObjectForRendering();
        }
        else if (activeTransformChanged) {
            queueActiveObjectTransformForRendering();
        }
    }

    void Application::queueActiveObjectTransformForRendering() {
        if (!activeSceneObjectId_) {
            return;
        }

        const SceneObject* object =
            scene_.findObject(*activeSceneObjectId_);
        if (object == nullptr) {
            return;
        }

        RenderCommand transformCommand;
        transformCommand.type = RenderCommandType::SetModelMatrix;
        transformCommand.data = object->getTransform().getModelMatrix();
        vk_.getRenderQueue().enqueue(std::move(transformCommand));
    }

    void Application::queueActiveObjectForRendering() {
        PointCloud pointCloud;
        glm::mat4 modelMatrix(1.0f);

        if (activeSceneObjectId_) {
            const SceneObject* object =
                scene_.findObject(*activeSceneObjectId_);

            if (object != nullptr) {
                pointCloud = object->getPointCloud();
                modelMatrix = object->getTransform().getModelMatrix();
            }
            else {
                activeSceneObjectId_.reset();
            }
        }

        RenderCommand pointCloudCommand;
        pointCloudCommand.type = RenderCommandType::UpdatePointCloud;
        pointCloudCommand.data = std::move(pointCloud);
        vk_.getRenderQueue().enqueue(std::move(pointCloudCommand));

        RenderCommand transformCommand;
        transformCommand.type = RenderCommandType::SetModelMatrix;
        transformCommand.data = modelMatrix;
        vk_.getRenderQueue().enqueue(std::move(transformCommand));

        float animationSpeed = 1.0f;
        if (activeSceneObjectId_) {
            const SceneObject* object =
                scene_.findObject(*activeSceneObjectId_);
            if (object != nullptr) {
                animationSpeed = object->getAnimationSpeed();
            }
        }

        RenderCommand animationCommand;
        animationCommand.type = RenderCommandType::SetAnimationSpeed;
        animationCommand.data = animationSpeed;
        vk_.getRenderQueue().enqueue(std::move(animationCommand));
    }

    void Application::queueRenderSettingsForRendering(
        const SceneRenderSettings& settings
    ) {
        const auto enqueueFloat = [this](RenderCommandType type, float value) {
            RenderCommand command;
            command.type = type;
            command.data = value;
            vk_.getRenderQueue().enqueue(std::move(command));
        };

        const auto enqueueBool = [this](RenderCommandType type, bool value) {
            RenderCommand command;
            command.type = type;
            command.data = value;
            vk_.getRenderQueue().enqueue(std::move(command));
        };

        enqueueFloat(
            RenderCommandType::SetMorphDuration,
            settings.morphDurationSeconds
        );
        enqueueFloat(
            RenderCommandType::SetVibrationAmplitude,
            settings.vibrationAmplitude
        );
        enqueueFloat(
            RenderCommandType::SetVibrationSpeed,
            settings.vibrationSpeed
        );
        enqueueBool(
            RenderCommandType::SetVibrationEnabled,
            settings.vibrationEnabled
        );
        enqueueBool(
            RenderCommandType::SetMorphingEnabled,
            settings.morphingEnabled
        );
        enqueueBool(
            RenderCommandType::SetGroundReflectionEnabled,
            settings.groundReflectionEnabled
        );
    }

    void Application::updateObjectDrag(
        GLFWwindow* window,
        bool inputCaptured
    ) {
        if (
            inputCaptured ||
            !activeSceneObjectId_ ||
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS
        ) {
            objectDragging_ = false;
            return;
        }

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        const glm::vec2 currentPosition(
            static_cast<float>(mouseX),
            static_cast<float>(mouseY)
        );

        if (!objectDragging_) {
            objectDragging_ = true;
            lastObjectDragPosition_ = currentPosition;
            return;
        }

        const glm::vec2 delta = currentPosition - lastObjectDragPosition_;
        lastObjectDragPosition_ = currentPosition;

        if (glm::length(delta) < 0.01f) {
            return;
        }

        const SceneObject* object =
            scene_.findObject(*activeSceneObjectId_);
        if (object == nullptr) {
            objectDragging_ = false;
            return;
        }

        Transform transform = object->getTransform();
        glm::vec3 rotation = transform.getRotation();
        constexpr float degreesPerPixel = 0.25f;
        rotation.x = std::remainder(
            rotation.x + delta.y * degreesPerPixel,
            360.0f
        );
        rotation.y = std::remainder(
            rotation.y + delta.x * degreesPerPixel,
            360.0f
        );
        transform.setRotation(rotation);

        sceneCommandQueue_.enqueue(
            SetSceneObjectTransformCommand{
                *activeSceneObjectId_,
                transform
            }
        );
    }

    void Application::run() {
        try {
            vk_.init([this](GLFWwindow* window) {
                controller_.init(window);
            });
            queueRenderSettingsForRendering(scene_.getRenderSettings());

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

                sceneCommandQueue_.enqueue(
                    UpsertPointCloudCommand{
                        filename,
                        std::move(renderCloud)
                    }
                );
            };

            terminal_.setLoadCallback(
                [this, &queueShape](
                    const std::string& filename
                ) {
                    try {
                        queueShape(filename);
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

            queueShape(
                "data/objects/shape/cube.json"
            );

            auto previousFrameTime = std::chrono::steady_clock::now();

            while (
                !terminal_.shouldExit() &&
                !vk_.getWindow().shouldClose()
            ) {
                const auto frameStart = std::chrono::steady_clock::now();
                glfwPollEvents();

                vk_.beginEditorFrame();
                const float deltaTime = std::clamp(
                    std::chrono::duration<float>(frameStart - previousFrameTime).count(),
                    0.0f,
                    0.1f
                );
                previousFrameTime = frameStart;

                const ImGuiIO& io = ImGui::GetIO();
                if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
                    controller_.update(
                        vk_.getWindow().getGLFWwindow(),
                        deltaTime
                    );
                }
                updateObjectDrag(
                    vk_.getWindow().getGLFWwindow(),
                    io.WantCaptureMouse || io.WantCaptureKeyboard
                );

                editorPanel_.draw(
                    activeSceneObjectId_,
                    queueShape,
                    camera_
                );

                processSceneCommands();

                if (
                    autoMorphEnabled_ &&
                    scene_.getRenderSettings().morphingEnabled &&
                    hasAutoMorphShape_ &&
                    std::chrono::steady_clock::now() >= nextAutoMorphTime_
                ) {
                    autoMorphShapeIndex_ =
                        (autoMorphShapeIndex_ + 1) % AUTO_MORPH_SHAPES.size();
                    try {
                        queueShape(
                            std::string(
                                AUTO_MORPH_SHAPES[autoMorphShapeIndex_]
                            )
                        );
                        nextAutoMorphTime_ =
                            std::chrono::steady_clock::now() +
                            AUTO_MORPH_INTERVAL;
                    }
                    catch (const std::exception& e) {
                        std::cerr
                            << "[Auto Morph] Failed to load shape: "
                            << e.what()
                            << '\n';
                        nextAutoMorphTime_ =
                            std::chrono::steady_clock::now() +
                            std::chrono::seconds(1);
                    }
                }

                vk_.drawFrame(camera_);

                std::this_thread::sleep_until(
                    frameStart + std::chrono::microseconds(16667)
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
