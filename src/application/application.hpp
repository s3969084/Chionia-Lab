
#pragma once

#include "backend/gpu_api/vulkan/VulkanBackend.hpp"
#include "ui/terminal_window.hpp"
#include "scene/camera_controller.hpp"
#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "core/commands/scene_command_queue.hpp"
#include "editor/editor_panel.hpp"

#include <glm/glm.hpp>
#include <chrono>
#include <cstddef>
#include <optional>

namespace chionia {

    class Application {
    public:
        Application();
        ~Application();

        void run();

    private:
        VulkanBackend vk_;
        TerminalWindow terminal_;

        Camera camera_;
        CameraController controller_{camera_};

        Scene scene_;
        SceneCommandQueue sceneCommandQueue_;
        EditorPanel editorPanel_{scene_, sceneCommandQueue_};
        std::optional<SceneObjectId> activeSceneObjectId_;
        bool objectDragging_ = false;
        glm::vec2 lastObjectDragPosition_{-1.0f};
        bool autoMorphEnabled_ = false;
        bool hasAutoMorphShape_ = false;
        std::size_t autoMorphShapeIndex_ = 0;
        std::chrono::steady_clock::time_point nextAutoMorphTime_{};

        void processSceneCommands();
        void updateObjectDrag(GLFWwindow* window, bool inputCaptured);
        void queueActiveObjectForRendering(bool instant = false);
        void queueActiveObjectTransformForRendering();
        void queueRenderSettingsForRendering(
            const SceneRenderSettings& settings
        );
    };

}
