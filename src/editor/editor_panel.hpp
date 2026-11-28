#pragma once

#include "core/commands/scene_command_queue.hpp"
#include "lesson/anatomy_lesson.hpp"
#include "lesson/biology_lesson.hpp"
#include "lesson/solar_system_lesson.hpp"
#include "lesson/test_lesson.hpp"
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

        bool isPresentationTabActive() const;

        void draw(
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&, bool)>& loadShape,
            const Camera& visualizationCamera,
            const Camera& presentationCamera,
            CameraController& visualizationCameraController,
            CameraController& presentationCameraController,
            float activeAnimationTime
        );

    private:
        enum class Tab {
            Visualization,
            Presentation,
            Settings,
            Performance
        };

        enum class PresentationLabelMode {
            SystemObjects,
            AllObjects,
            Hidden
        };

        enum class PresentationSubject {
            None,
            SolarSystem,
            HumanCell,
            Anatomy,
            AdditionalLesson
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
            std::optional<SceneObjectId> activeObjectId,
            CameraController& cameraController,
            float viewportHeight
        );
        void restorePresentationView(
            CameraController& cameraController,
            bool selectVisualizationObject = true
        );
        const char* activePresentationKey() const;
        bool hasActivePresentation() const;
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
            const Camera& camera,
            float animationTime
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
        BiologyLesson biologyLesson_;
        AnatomyLesson anatomyLesson_;
        TestLesson testLesson_;
        PresentationSubject activePresentationSubject_ =
            PresentationSubject::None;
        int testLessonExampleIndex_ = 0;
        std::array<char, 2048> lessonTranscript_{};
        std::string lessonFeedback_ =
            "Paste one spoken segment, then apply it to the scene.";
        std::optional<SceneRenderSettings> settingsBeforeLesson_;
        std::optional<SceneRenderSettings> presentationRenderSettings_;
        std::optional<SceneObjectId> visualizationObjectBeforePresentation_;
        bool returnToInfinityAfterClear_ = false;
        bool presentationCameraInitialized_ = false;
        bool editorLayerVisible_ = true;
        PresentationLabelMode presentationLabelMode_ =
            PresentationLabelMode::SystemObjects;
        bool cinematicCameraEnabled_ = true;
        bool shapeOutlinesEnabled_ = true;
    };

}
