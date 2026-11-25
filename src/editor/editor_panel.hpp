#pragma once

#include "core/commands/scene_command_queue.hpp"
#include "lesson/solar_system_lesson.hpp"
#include "scene/procedural/infinity_generator.hpp"
#include "scene/camera.hpp"
#include "scene/camera_controller.hpp"
#include "scene/scene.hpp"

#include <functional>
#include <array>
#include <optional>
#include <string>

namespace chionia {

    class EditorPanel {
    public:
        EditorPanel(Scene& scene, SceneCommandQueue& commandQueue);

        void draw(
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&, bool)>& loadShape,
            const Camera& camera,
            CameraController& cameraController
        );

    private:
        enum class Tab {
            Visualization,
            Presentation,
            Shapes,
            Settings,
            Performance
        };

        enum class PresentationLabelMode {
            SystemObjects,
            AllObjects,
            Hidden
        };

        struct ShapeEntry {
            const char* label;
            const char* assetPath;
            const char* icon;
        };

        static const ShapeEntry* getShapeEntries(std::size_t& count);

        void drawBackground(
            float width,
            float height,
            std::optional<SceneObjectId> activeObjectId,
            const Camera& camera
        );
        void drawTopBar(float width);
        void enterPresentationMode(
            CameraController& cameraController,
            float viewportHeight
        );
        void drawSidebar(
            float height,
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&, bool)>& loadShape,
            CameraController& cameraController
        );
        void drawLessonAnnotations(
            float width,
            float height,
            std::optional<SceneObjectId> activeObjectId,
            const Camera& camera
        );
        void drawShapeLibrary(
            float availableHeight,
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&, bool)>& loadShape
        );
        void drawVisualizationControls();
        void drawPresentationControls(CameraController& cameraController);
        void drawProceduralControls();
        void drawObjectSettings(std::optional<SceneObjectId> activeObjectId);
        void drawPerformance(std::optional<SceneObjectId> activeObjectId);
        void drawFooter(
            float width,
            float height,
            std::optional<SceneObjectId> activeObjectId
        );

        Scene& scene_;
        SceneCommandQueue& commandQueue_;
        Tab activeTab_ = Tab::Visualization;
        InfinityGeneratorConfig infinityConfig_;
        SolarSystemLesson solarSystemLesson_;
        std::array<char, 2048> lessonTranscript_{};
        std::string lessonFeedback_ =
            "Paste one spoken segment, then apply it to the scene.";
        std::optional<SceneRenderSettings> settingsBeforeLesson_;
        std::optional<float> cameraDistanceBeforeLesson_;
        std::optional<glm::vec3> cameraTargetBeforeLesson_;
        PresentationLabelMode presentationLabelMode_ =
            PresentationLabelMode::SystemObjects;
        bool cinematicCameraEnabled_ = true;
        bool shapeOutlinesEnabled_ = true;
    };

}
