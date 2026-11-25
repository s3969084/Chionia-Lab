
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

        using OutlinePath = std::vector<glm::vec3>;

        std::vector<OutlinePath> getShapeOutlinePaths(
            const std::string& filename,
            const PointCloudBounds& bounds
        ) {
            const glm::vec3 center =
                (bounds.minimum + bounds.maximum) * 0.5f;
            std::vector<OutlinePath> paths;

            if (filename.find("cube.json") != std::string::npos) {
                std::array<glm::vec3, 8> corners;
                for (std::size_t i = 0; i < corners.size(); ++i) {
                    corners[i] = glm::vec3(
                        (i & 1U) == 0 ? bounds.minimum.x : bounds.maximum.x,
                        (i & 2U) == 0 ? bounds.minimum.y : bounds.maximum.y,
                        (i & 4U) == 0 ? bounds.minimum.z : bounds.maximum.z
                    );
                }

                for (std::size_t i = 0; i < corners.size(); ++i) {
                    for (std::size_t axis = 0; axis < 3; ++axis) {
                        const std::size_t adjacent = i ^ (1U << axis);
                        if (i < adjacent) {
                            paths.push_back({corners[i], corners[adjacent]});
                        }
                    }
                }
            }
            else if (filename.find("pyramid.json") != std::string::npos) {
                const std::array<glm::vec3, 4> base = {{
                    {bounds.minimum.x, bounds.minimum.y, bounds.minimum.z},
                    {bounds.maximum.x, bounds.minimum.y, bounds.minimum.z},
                    {bounds.maximum.x, bounds.minimum.y, bounds.maximum.z},
                    {bounds.minimum.x, bounds.minimum.y, bounds.maximum.z}
                }};
                const glm::vec3 apex(center.x, bounds.maximum.y, center.z);

                for (std::size_t i = 0; i < base.size(); ++i) {
                    paths.push_back({base[i], base[(i + 1) % base.size()]});
                    paths.push_back({base[i], apex});
                }
            }
            else if (filename.find("cone.json") != std::string::npos) {
                constexpr std::size_t RING_SEGMENTS = 48;
                constexpr std::size_t SPOKE_COUNT = 8;
                const float radiusX =
                    (bounds.maximum.x - bounds.minimum.x) * 0.5f;
                const float radiusZ =
                    (bounds.maximum.z - bounds.minimum.z) * 0.5f;

                OutlinePath baseRing;
                baseRing.reserve(RING_SEGMENTS + 1);
                for (std::size_t i = 0; i <= RING_SEGMENTS; ++i) {
                    const float angle =
                        static_cast<float>(i) /
                        static_cast<float>(RING_SEGMENTS) * 6.28318530718f;
                    baseRing.emplace_back(
                        center.x + std::cos(angle) * radiusX,
                        bounds.minimum.y,
                        center.z + std::sin(angle) * radiusZ
                    );
                }
                paths.push_back(std::move(baseRing));

                const glm::vec3 apex(center.x, bounds.maximum.y, center.z);
                for (std::size_t i = 0; i < SPOKE_COUNT; ++i) {
                    const float angle =
                        static_cast<float>(i) /
                        static_cast<float>(SPOKE_COUNT) * 6.28318530718f;
                    const glm::vec3 basePoint(
                        center.x + std::cos(angle) * radiusX,
                        bounds.minimum.y,
                        center.z + std::sin(angle) * radiusZ
                    );
                    paths.push_back({basePoint, apex});
                }
            }

            return paths;
        }

        std::vector<glm::vec3> sampleOutlinePaths(
            const std::vector<OutlinePath>& paths,
            std::size_t pointCount
        ) {
            struct MeasuredPath {
                const OutlinePath* points = nullptr;
                std::vector<double> cumulativeLength;
                double length = 0.0;
                std::size_t sampleCount = 0;
                double fractionalRemainder = 0.0;
            };

            std::vector<MeasuredPath> measuredPaths;
            measuredPaths.reserve(paths.size());
            double totalLength = 0.0;

            for (const OutlinePath& path : paths) {
                if (path.size() < 2) {
                    continue;
                }

                MeasuredPath measured;
                measured.points = &path;
                measured.cumulativeLength.push_back(0.0);
                for (std::size_t i = 1; i < path.size(); ++i) {
                    measured.length += glm::length(path[i] - path[i - 1]);
                    measured.cumulativeLength.push_back(measured.length);
                }
                if (measured.length <= 0.0) {
                    continue;
                }
                totalLength += measured.length;
                measuredPaths.push_back(std::move(measured));
            }

            if (measuredPaths.empty() || pointCount == 0 || totalLength <= 0.0) {
                return {};
            }

            std::size_t assignedCount = 0;
            for (MeasuredPath& path : measuredPaths) {
                const double exactCount =
                    static_cast<double>(pointCount) * path.length / totalLength;
                path.sampleCount = std::max<std::size_t>(
                    2,
                    static_cast<std::size_t>(std::floor(exactCount))
                );
                path.fractionalRemainder = exactCount - std::floor(exactCount);
                assignedCount += path.sampleCount;
            }

            while (assignedCount < pointCount) {
                auto next = std::max_element(
                    measuredPaths.begin(),
                    measuredPaths.end(),
                    [](const MeasuredPath& a, const MeasuredPath& b) {
                        return a.fractionalRemainder < b.fractionalRemainder;
                    }
                );
                ++next->sampleCount;
                next->fractionalRemainder = -1.0;
                ++assignedCount;
            }

            while (assignedCount > pointCount) {
                auto next = std::max_element(
                    measuredPaths.begin(),
                    measuredPaths.end(),
                    [](const MeasuredPath& a, const MeasuredPath& b) {
                        return a.sampleCount < b.sampleCount;
                    }
                );
                if (next == measuredPaths.end() || next->sampleCount <= 2) {
                    break;
                }
                --next->sampleCount;
                --assignedCount;
            }

            std::vector<glm::vec3> result;
            result.reserve(pointCount);
            for (const MeasuredPath& path : measuredPaths) {
                for (std::size_t i = 0; i < path.sampleCount; ++i) {
                    const double fraction =
                        path.sampleCount == 1
                        ? 0.0
                        : static_cast<double>(i) /
                            static_cast<double>(path.sampleCount - 1);
                    const double targetDistance = fraction * path.length;
                    const auto upper = std::lower_bound(
                        path.cumulativeLength.begin(),
                        path.cumulativeLength.end(),
                        targetDistance
                    );
                    if (upper == path.cumulativeLength.begin()) {
                        result.push_back(path.points->front());
                        continue;
                    }
                    if (upper == path.cumulativeLength.end()) {
                        result.push_back(path.points->back());
                        continue;
                    }

                    const std::size_t segment = static_cast<std::size_t>(
                        upper - path.cumulativeLength.begin()
                    ) - 1;
                    const double startDistance = path.cumulativeLength[segment];
                    const double segmentLength =
                        path.cumulativeLength[segment + 1] - startDistance;
                    const float segmentFraction = segmentLength > 0.0
                        ? static_cast<float>(
                            (targetDistance - startDistance) / segmentLength
                        )
                        : 0.0f;
                    result.push_back(glm::mix(
                        (*path.points)[segment],
                        (*path.points)[segment + 1],
                        segmentFraction
                    ));
                }
            }
            return result;
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
        enqueueBool(
            RenderCommandType::SetEnergyEffectsEnabled,
            settings.energyEffectsEnabled
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

            bool shapeOutlinesEnabled = true;
            auto queueShape = [this, &shapeOutlinesEnabled](
                const std::string& filename,
                std::optional<bool> outlineOverride = std::nullopt
            ) {
                if (outlineOverride.has_value()) {
                    shapeOutlinesEnabled = *outlineOverride;
                }

                PointCloud sourceCloud =
                    PointCloudLoader::loadPointCloudFromFile(filename);

                const std::size_t sourcePointCount =
                    sourceCloud.getPointCount();
                const auto sourceBounds = sourceCloud.getBounds();

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

                std::vector<glm::vec3> outlinePositions;
                if (shapeOutlinesEnabled && sourceBounds) {
                    const auto outlinePaths = getShapeOutlinePaths(
                        filename,
                        *sourceBounds
                    );
                    if (!outlinePaths.empty()) {
                        const std::size_t outlinePointCount = std::min(
                            ShapeResampler::PARTICLE_COUNT * 6 / 100,
                            ShapeResampler::PARTICLE_COUNT / 4
                        );
                        outlinePositions = sampleOutlinePaths(
                            outlinePaths,
                            outlinePointCount
                        );
                        if (outlinePositions.size() == outlinePointCount) {
                            std::copy(
                                outlinePositions.begin(),
                                outlinePositions.end(),
                                targetPositions.end() -
                                    static_cast<std::ptrdiff_t>(outlinePointCount)
                            );
                        }
                        else {
                            outlinePositions.clear();
                        }
                    }
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

                const std::size_t firstOutlinePoint =
                    targetPositions.size() - outlinePositions.size();
                for (std::size_t i = 0; i < targetPositions.size(); ++i) {
                    const bool isOutlinePoint = i >= firstOutlinePoint;
                    renderCloud.addPoint(
                        targetPositions[i],
                        isOutlinePoint
                            ? glm::vec4(0.0f, 0.95f, 1.0f, 1.0f)
                            : glm::vec4(1.0f)
                    );
                }

                std::cout
                    << "\n[Shape Loader] Loaded: " << filename
                    << "\n[Shape Loader] Source points: "
                    << sourcePointCount
                    << "\n[Shape Loader] Resampled points: "
                    << renderCloud.getPointCount()
                    << "\n[Shape Loader] Outline points: "
                    << outlinePositions.size()
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
                controller_.update(
                    vk_.getWindow().getGLFWwindow(),
                    deltaTime,
                    !io.WantCaptureMouse && !io.WantCaptureKeyboard
                );
                updateObjectDrag(
                    vk_.getWindow().getGLFWwindow(),
                    io.WantCaptureMouse || io.WantCaptureKeyboard
                );

                editorPanel_.draw(
                    activeSceneObjectId_,
                    queueShape,
                    camera_,
                    controller_
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
