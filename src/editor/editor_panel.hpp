#pragma once

#include "core/commands/scene_command_queue.hpp"
#include "scene/procedural/infinity_generator.hpp"
#include "scene/camera.hpp"
#include "scene/scene.hpp"

#include <functional>
#include <optional>
#include <string>

namespace chionia {

    class EditorPanel {
    public:
        EditorPanel(Scene& scene, SceneCommandQueue& commandQueue);

        void draw(
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&)>& loadShape,
            const Camera& camera
        );

    private:
        enum class Tab {
            Visualization,
            Shapes,
            Settings,
            Performance
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
        void drawSidebar(
            float height,
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&)>& loadShape
        );
        void drawShapeLibrary(
            float availableHeight,
            std::optional<SceneObjectId> activeObjectId,
            const std::function<void(const std::string&)>& loadShape
        );
        void drawVisualizationControls();
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
    };

}
