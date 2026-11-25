#include "editor_panel.hpp"
#include "scene/ground_plane.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace chionia {

    namespace {

        constexpr ImVec4 CYAN(0.10f, 0.78f, 1.00f, 1.00f);
        constexpr ImVec4 MUTED(0.57f, 0.67f, 0.79f, 1.00f);

        constexpr ImGuiWindowFlags OVERLAY_FLAGS =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus;

        bool containsAsset(const std::string& key, const char* path) {
            return key.find(path) != std::string::npos;
        }

        bool drawTabButton(
            const char* label,
            bool selected,
            const ImVec2& size
        ) {
            if (selected) {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4(0.03f, 0.28f, 0.48f, 0.95f)
                );
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonHovered,
                    ImVec4(0.04f, 0.40f, 0.68f, 1.00f)
                );
            }
            else {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4(0.02f, 0.07f, 0.13f, 0.58f)
                );
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonHovered,
                    ImVec4(0.04f, 0.18f, 0.30f, 0.95f)
                );
            }

            ImGui::PushStyleColor(
                ImGuiCol_ButtonActive,
                ImVec4(0.02f, 0.42f, 0.69f, 1.00f)
            );
            const bool clicked = ImGui::Button(label, size);
            ImGui::PopStyleColor(3);
            return clicked;
        }

        void drawSectionTitle(const char* title) {
            ImGui::TextUnformatted(title);
            ImGui::Spacing();
        }

        void drawInfinityMark(
            ImDrawList* drawList,
            const ImVec2& origin,
            float width,
            float height
        ) {
            const ImVec2 left(origin.x, origin.y + height * 0.5f);
            const ImVec2 center(origin.x + width * 0.5f, origin.y + height * 0.5f);
            const ImVec2 right(origin.x + width, origin.y + height * 0.5f);
            const float loopX = width * 0.42f;
            const float loopY = height * 0.42f;

            const ImVec2 leftUpperA(left.x + loopX, left.y - loopY);
            const ImVec2 leftUpperB(center.x - loopX, center.y - loopY);
            const ImVec2 rightLowerA(center.x + loopX, center.y + loopY);
            const ImVec2 rightLowerB(right.x - loopX, right.y + loopY);
            const ImVec2 rightUpperA(right.x - loopX, right.y - loopY);
            const ImVec2 rightUpperB(center.x + loopX, center.y - loopY);
            const ImVec2 leftLowerA(center.x - loopX, center.y + loopY);
            const ImVec2 leftLowerB(left.x + loopX, left.y + loopY);

            const auto drawPath = [&](ImU32 color, float thickness) {
                drawList->AddBezierCubic(
                    left, leftUpperA, leftUpperB, center,
                    color, thickness, 16
                );
                drawList->AddBezierCubic(
                    center, rightLowerA, rightLowerB, right,
                    color, thickness, 16
                );
                drawList->AddBezierCubic(
                    right, rightUpperA, rightUpperB, center,
                    color, thickness, 16
                );
                drawList->AddBezierCubic(
                    center, leftLowerA, leftLowerB, left,
                    color, thickness, 16
                );
            };

            drawPath(IM_COL32(0, 133, 255, 80), 10.0f);
            drawPath(IM_COL32(48, 210, 255, 245), 4.5f);
        }

    }

    EditorPanel::EditorPanel(
        Scene& scene,
        SceneCommandQueue& commandQueue
    )
        : scene_(scene),
          commandQueue_(commandQueue) {
    }

    const EditorPanel::ShapeEntry* EditorPanel::getShapeEntries(
        std::size_t& count
    ) {
        static const std::array<ShapeEntry, 9> entries = {{
            {"Möbius", "shape/mobius.json", "M"},
            {"Superquadric", "shape/superquadric.json", "Sq"},
            {"Double", "shape/double.json", "D"},
            {"Cone", "shape/cone.json", "C"},
            {"Helix", "shape/helix.json", "H"},
            {"Cube", "shape/cube.json", "Cu"},
            {"Pyramid", "shape/pyramid.json", "P"},
            {"Sphere", "shape/sphere.json", "S"},
            {"Trefoil Knot", "shape/trefoil_knot.json", "T"}
        }};

        count = entries.size();
        return entries.data();
    }

    void EditorPanel::draw(
        std::optional<SceneObjectId> activeObjectId,
        const std::function<void(const std::string&)>& loadShape,
        const Camera& camera
    ) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 size = viewport->Size;

        drawBackground(size.x, size.y, activeObjectId, camera);
        drawTopBar(size.x);
        drawSidebar(size.y, activeObjectId, loadShape);
        drawFooter(size.x, size.y, activeObjectId);
    }

    void EditorPanel::drawBackground(
        float width,
        float height,
        std::optional<SceneObjectId> activeObjectId,
        const Camera& camera
    ) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const glm::mat4 view = camera.getViewMatrix();
        const glm::mat4 projection = camera.getProjectionMatrix(
            width / height,
            true
        );
        glm::vec3 groundCenter(0.0f);
        if (activeObjectId) {
            const SceneObject* activeObject =
                scene_.findObject(*activeObjectId);
            if (activeObject != nullptr) {
                const glm::vec3& objectPosition =
                    activeObject->getTransform().getPosition();
                groundCenter.x = objectPosition.x;
                groundCenter.z = objectPosition.z;
            }
        }
        const float cameraDistanceToGround = glm::length(
            camera.getTransform().getPosition() -
            glm::vec3(
                groundCenter.x,
                SCENE_GROUND_HEIGHT,
                groundCenter.z
            )
        );
        const float outerOrbitRadius = std::min(
            0.95f,
            cameraDistanceToGround * 0.45f
        );
        const float innerOrbitRadius = outerOrbitRadius * 0.82f;

        const auto projectGroundPoint = [&] (
            float x,
            float z,
            ImVec2& screenPosition
        ) {
            const glm::vec4 clipPosition = projection * view * glm::vec4(
                groundCenter.x + x,
                SCENE_GROUND_HEIGHT,
                groundCenter.z + z,
                1.0f
            );
            if (clipPosition.w <= 0.08f) {
                return false;
            }

            const float inverseW = 1.0f / clipPosition.w;
            const float normalizedX = clipPosition.x * inverseW;
            const float normalizedY = clipPosition.y * inverseW;

            screenPosition = ImVec2(
                viewport->Pos.x + (normalizedX + 1.0f) * width * 0.5f,
                viewport->Pos.y + (normalizedY + 1.0f) * height * 0.5f
            );
            return std::isfinite(screenPosition.x) &&
                std::isfinite(screenPosition.y);
        };

        for (int i = 0; i < 48; ++i) {
            const float x = viewport->Pos.x +
                std::fmod(static_cast<float>(i * 173 + 41), width);
            const float y = viewport->Pos.y +
                std::fmod(static_cast<float>(i * 83 + 19), height * 0.52f);
            const int alpha = 16 + (i * 13) % 31;
            drawList->AddCircleFilled(
                ImVec2(x, y),
                (i % 5 == 0) ? 1.7f : 1.0f,
                IM_COL32(46, 129, 255, alpha)
            );
        }

        const auto drawGroundGridLine = [&] (
            bool variesX,
            float fixedCoordinate,
            ImU32 color
        ) {
            constexpr int SAMPLE_COUNT = 64;
            constexpr float GRID_HALF_EXTENT = 8.0f;
            ImVec2 previousPoint{};
            bool hasPreviousPoint = false;

            for (int sample = 0; sample <= SAMPLE_COUNT; ++sample) {
                const float t = static_cast<float>(sample) /
                    static_cast<float>(SAMPLE_COUNT);
                const float varyingCoordinate =
                    -GRID_HALF_EXTENT + 2.0f * GRID_HALF_EXTENT * t;
                const float x = variesX
                    ? varyingCoordinate
                    : fixedCoordinate;
                const float z = variesX
                    ? fixedCoordinate
                    : varyingCoordinate;

                ImVec2 currentPoint{};
                if (!projectGroundPoint(x, z, currentPoint)) {
                    hasPreviousPoint = false;
                    continue;
                }

                if (hasPreviousPoint) {
                    drawList->AddLine(
                        previousPoint,
                        currentPoint,
                        color,
                        1.0f
                    );
                }

                previousPoint = currentPoint;
                hasPreviousPoint = true;
            }
        };

        for (int i = -9; i <= 9; ++i) {
            drawGroundGridLine(
                false,
                static_cast<float>(i) * 0.32f,
                IM_COL32(0, 115, 235, 34)
            );
        }

        for (int i = -10; i <= 20; ++i) {
            drawGroundGridLine(
                true,
                static_cast<float>(i) * 0.4f,
                IM_COL32(0, 104, 220, 27)
            );
        }

        const auto drawOrbit = [&](
            float radius,
            ImU32 color
        ) {
            std::array<ImVec2, 97> points{};
            for (std::size_t i = 0; i < points.size(); ++i) {
                const float angle =
                    static_cast<float>(i) * 6.28318530718f /
                    static_cast<float>(points.size() - 1);
                projectGroundPoint(
                    std::cos(angle) * radius,
                    std::sin(angle) * radius,
                    points[i]
                );
            }
            drawList->AddPolyline(
                points.data(),
                static_cast<int>(points.size()),
                color,
                ImDrawFlags_None,
                1.15f
            );
        };

        drawOrbit(innerOrbitRadius, IM_COL32(0, 140, 255, 68));
        drawOrbit(outerOrbitRadius, IM_COL32(123, 46, 255, 54));

        const float orbitAngle = static_cast<float>(ImGui::GetTime()) * 0.72f;
        ImVec2 orbitMarker{};
        projectGroundPoint(
            std::cos(orbitAngle) * outerOrbitRadius,
            std::sin(orbitAngle) * outerOrbitRadius,
            orbitMarker
        );
        ImVec2 orbitTrail{};
        projectGroundPoint(
            std::cos(orbitAngle - 0.075f) * outerOrbitRadius,
            std::sin(orbitAngle - 0.075f) * outerOrbitRadius,
            orbitTrail
        );
        const float glowPulse = 0.5f + 0.5f * std::sin(
            static_cast<float>(ImGui::GetTime()) * 5.0f
        );
        drawList->AddLine(
            orbitTrail,
            orbitMarker,
            IM_COL32(20, 200, 255, 90),
            2.0f
        );
        drawList->AddCircleFilled(
            orbitMarker,
            14.0f + glowPulse * 2.0f,
            IM_COL32(0, 170, 255, 11 + static_cast<int>(glowPulse * 9.0f))
        );
        drawList->AddCircleFilled(
            orbitMarker,
            8.5f,
            IM_COL32(0, 195, 255, 32 + static_cast<int>(glowPulse * 16.0f))
        );
        drawList->AddCircleFilled(
            orbitMarker,
            4.5f,
            IM_COL32(0, 218, 255, 90)
        );
        drawList->AddCircle(
            orbitMarker,
            6.0f,
            IM_COL32(91, 231, 255, 175),
            20,
            1.2f
        );
        drawList->AddCircleFilled(
            orbitMarker,
            2.4f,
            IM_COL32(0, 220, 255, 245)
        );
        drawList->AddCircleFilled(
            orbitMarker,
            1.1f,
            IM_COL32(225, 252, 255, 255)
        );
    }

    void EditorPanel::drawTopBar(float width) {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(width, 132.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            ImVec4(0.004f, 0.012f, 0.026f, 0.88f)
        );

        if (ImGui::Begin("##ChioniaTopBar", nullptr, OVERLAY_FLAGS)) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(
                ImVec2(16.0f, 48.0f),
                ImVec2(width - 16.0f, 128.0f),
                IM_COL32(4, 96, 166, 148),
                14.0f,
                0,
                1.0f
            );

            drawInfinityMark(drawList, ImVec2(36.0f, 8.0f), 53.0f, 37.0f);
            ImGui::SetCursorPos(ImVec2(105.0f, 14.0f));
            ImGui::SetWindowFontScale(1.55f);
            ImGui::TextUnformatted("Chionia Lab");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::SetCursorPos(ImVec2(36.0f, 61.0f));
            if (drawTabButton(
                "Visualization",
                activeTab_ == Tab::Visualization,
                ImVec2(146.0f, 44.0f)
            )) {
                activeTab_ = Tab::Visualization;
            }
            ImGui::SameLine(0.0f, 7.0f);
            if (drawTabButton(
                "Shapes",
                activeTab_ == Tab::Shapes,
                ImVec2(82.0f, 44.0f)
            )) {
                activeTab_ = Tab::Shapes;
            }
            ImGui::SameLine(0.0f, 7.0f);
            if (drawTabButton(
                "Settings",
                activeTab_ == Tab::Settings,
                ImVec2(92.0f, 44.0f)
            )) {
                activeTab_ = Tab::Settings;
            }
            ImGui::SameLine(0.0f, 7.0f);
            if (drawTabButton(
                "Performance",
                activeTab_ == Tab::Performance,
                ImVec2(112.0f, 44.0f)
            )) {
                activeTab_ = Tab::Performance;
            }

            if (width > 720.0f) {
                ImGui::SetCursorPos(ImVec2(width - 177.0f, 76.0f));
                char fpsText[32];
                std::snprintf(
                    fpsText,
                    sizeof(fpsText),
                    "%.0f FPS",
                    ImGui::GetIO().Framerate
                );
                ImGui::TextColored(CYAN, "%s", fpsText);
                ImGui::SameLine(0.0f, 20.0f);
                ImGui::TextColored(MUTED, "GPU: Vulkan");
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void EditorPanel::drawSidebar(
        float height,
        std::optional<SceneObjectId> activeObjectId,
        const std::function<void(const std::string&)>& loadShape
    ) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float panelHeight = std::max(250.0f, height - 182.0f);

        ImGui::SetNextWindowPos(
            ImVec2(viewport->Pos.x + 16.0f, viewport->Pos.y + 118.0f)
        );
        ImGui::SetNextWindowSize(ImVec2(286.0f, panelHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(11.0f, 10.0f));
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            ImVec4(0.012f, 0.025f, 0.045f, 0.90f)
        );

        if (ImGui::Begin("##ChioniaSidebar", nullptr, OVERLAY_FLAGS)) {
            const float libraryHeight = activeTab_ == Tab::Visualization
                ? panelHeight * 0.44f
                : panelHeight * 0.55f;

            if (
                activeTab_ == Tab::Visualization ||
                activeTab_ == Tab::Shapes
            ) {
                drawShapeLibrary(libraryHeight, activeObjectId, loadShape);
            }

            if (activeTab_ == Tab::Visualization) {
                ImGui::Spacing();
                drawVisualizationControls();
            }
            else if (activeTab_ == Tab::Shapes) {
                ImGui::Spacing();
                drawProceduralControls();
            }
            else if (activeTab_ == Tab::Settings) {
                drawObjectSettings(activeObjectId);
            }
            else {
                drawPerformance(activeObjectId);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    void EditorPanel::drawShapeLibrary(
        float availableHeight,
        std::optional<SceneObjectId> activeObjectId,
        const std::function<void(const std::string&)>& loadShape
    ) {
        ImGui::BeginChild(
            "##ShapeLibrary",
            ImVec2(0.0f, availableHeight),
            true,
            ImGuiWindowFlags_None
        );

        drawSectionTitle("Shape Library");

        std::size_t count = 0;
        const ShapeEntry* entries = getShapeEntries(count);
        const std::size_t visibleCount = activeTab_ == Tab::Visualization
            ? std::min<std::size_t>(count, 6)
            : count;
        const SceneObject* activeObject = activeObjectId
            ? scene_.findObject(*activeObjectId)
            : nullptr;

        for (std::size_t i = 0; i < visibleCount; ++i) {
            const ShapeEntry& entry = entries[i];
            const bool selected =
                activeObject != nullptr &&
                containsAsset(activeObject->getKey(), entry.assetPath);

            ImGui::PushID(static_cast<int>(i));
            ImGui::PushStyleColor(
                ImGuiCol_Header,
                selected
                    ? ImVec4(0.03f, 0.24f, 0.41f, 0.96f)
                    : ImVec4(0.02f, 0.06f, 0.11f, 0.42f)
            );
            ImGui::PushStyleColor(
                ImGuiCol_HeaderHovered,
                ImVec4(0.03f, 0.19f, 0.32f, 0.92f)
            );

            ImGui::Selectable(
                "##ShapeRow",
                selected,
                ImGuiSelectableFlags_None,
                ImVec2(0.0f, 34.0f)
            );

            const ImVec2 rowMin = ImGui::GetItemRectMin();
            const ImVec2 rowMax = ImGui::GetItemRectMax();
            const ImVec2 iconMin(rowMin.x + 7.0f, rowMin.y + 4.0f);
            const ImVec2 iconMax(iconMin.x + 35.0f, rowMax.y - 4.0f);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(
                iconMin,
                iconMax,
                selected
                    ? IM_COL32(7, 99, 153, 235)
                    : IM_COL32(6, 34, 57, 230),
                6.0f
            );
            drawList->AddText(
                ImVec2(iconMin.x + 5.0f, iconMin.y + 5.0f),
                IM_COL32(107, 218, 255, 255),
                entry.icon
            );
            drawList->AddText(
                ImVec2(iconMax.x + 10.0f, iconMin.y + 5.0f),
                IM_COL32(220, 233, 246, 255),
                entry.label
            );

            if (ImGui::IsItemClicked() && loadShape) {
                const std::string path =
                    std::string("data/objects/") + entry.assetPath;
                loadShape(path);
            }

            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void EditorPanel::drawVisualizationControls() {
        ImGui::BeginChild("##VisualizationControls", ImVec2(0.0f, 0.0f), true);
        drawSectionTitle("Controls");

        SceneRenderSettings settings = scene_.getRenderSettings();
        bool changed = false;

        ImGui::TextUnformatted("Morph Speed");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 51.0f);
        ImGui::TextColored(CYAN, "%.1fs", settings.morphDurationSeconds);
        changed |= ImGui::SliderFloat(
            "##MorphSpeed",
            &settings.morphDurationSeconds,
            0.25f,
            5.0f,
            ""
        );

        ImGui::TextUnformatted("Vibration Amp");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 51.0f);
        ImGui::TextColored(CYAN, "%.2f", settings.vibrationAmplitude);
        changed |= ImGui::SliderFloat(
            "##VibrationAmplitude",
            &settings.vibrationAmplitude,
            0.0f,
            0.12f,
            ""
        );

        ImGui::TextUnformatted("Vibration Speed");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 51.0f);
        ImGui::TextColored(CYAN, "%.1f", settings.vibrationSpeed);
        changed |= ImGui::SliderFloat(
            "##VibrationSpeed",
            &settings.vibrationSpeed,
            0.0f,
            20.0f,
            ""
        );

        changed |= ImGui::Checkbox("Vibration", &settings.vibrationEnabled);
        changed |= ImGui::Checkbox("Morphing", &settings.morphingEnabled);
        ImGui::BeginDisabled(!settings.morphingEnabled);
        changed |= ImGui::Checkbox("Auto morph", &settings.autoMorphEnabled);
        ImGui::EndDisabled();
        changed |= ImGui::Checkbox(
            "Ground reflection",
            &settings.groundReflectionEnabled
        );

        if (changed) {
            commandQueue_.enqueue(
                SetSceneRenderSettingsCommand{settings}
            );
        }

        ImGui::EndChild();
    }

    void EditorPanel::drawProceduralControls() {
        ImGui::BeginChild("##ProceduralInfinity", ImVec2(0.0f, 0.0f), true);
        drawSectionTitle("Procedural Infinity");

        int pointCount = static_cast<int>(infinityConfig_.pointCount);
        if (ImGui::InputInt("Points", &pointCount, 1000, 5000)) {
            pointCount = std::clamp(pointCount, 3, 1000000);
            infinityConfig_.pointCount = static_cast<std::size_t>(pointCount);
        }

        ImGui::SliderFloat(
            "Thickness",
            &infinityConfig_.thickness,
            0.0f,
            0.15f,
            "%.3f"
        );

        if (ImGui::Button("Generate Infinity", ImVec2(-1.0f, 36.0f))) {
            PointCloud pointCloud =
                InfinityGenerator::generate(infinityConfig_);

            commandQueue_.enqueue(
                UpsertPointCloudCommand{
                    "procedural/infinity",
                    std::move(pointCloud)
                }
            );
        }

        ImGui::EndChild();
    }

    void EditorPanel::drawObjectSettings(
        std::optional<SceneObjectId> activeObjectId
    ) {
        ImGui::BeginChild("##ObjectSettings", ImVec2(0.0f, 0.0f), true);
        drawSectionTitle("Object Settings");

        if (!activeObjectId) {
            ImGui::TextColored(MUTED, "Select a shape to edit its transform.");
            ImGui::EndChild();
            return;
        }

        const SceneObject* object = scene_.findObject(*activeObjectId);
        if (object == nullptr) {
            ImGui::TextColored(MUTED, "The selected object is unavailable.");
            ImGui::EndChild();
            return;
        }

        Transform transform = object->getTransform();
        float position[3] = {
            transform.getPosition().x,
            transform.getPosition().y,
            transform.getPosition().z
        };
        if (ImGui::DragFloat3("Position", position, 0.02f)) {
            transform.setPosition(glm::vec3(position[0], position[1], position[2]));
            commandQueue_.enqueue(
                SetSceneObjectTransformCommand{*activeObjectId, transform}
            );
        }

        float rotation[3] = {
            transform.getRotation().x,
            transform.getRotation().y,
            transform.getRotation().z
        };
        if (ImGui::DragFloat3("Rotation", rotation, 0.25f)) {
            transform.setRotation(glm::vec3(rotation[0], rotation[1], rotation[2]));
            commandQueue_.enqueue(
                SetSceneObjectTransformCommand{*activeObjectId, transform}
            );
        }

        float scale[3] = {
            transform.getScale().x,
            transform.getScale().y,
            transform.getScale().z
        };
        if (ImGui::DragFloat3("Scale", scale, 0.01f, 0.05f, 10.0f)) {
            transform.setScale(glm::vec3(scale[0], scale[1], scale[2]));
            commandQueue_.enqueue(
                SetSceneObjectTransformCommand{*activeObjectId, transform}
            );
        }

        float animationSpeed = object->getAnimationSpeed();
        if (ImGui::SliderFloat("Animation Speed", &animationSpeed, 0.0f, 3.0f)) {
            commandQueue_.enqueue(
                SetSceneObjectAnimationSpeedCommand{
                    *activeObjectId,
                    animationSpeed
                }
            );
        }

        ImGui::Spacing();
        if (ImGui::Button("Remove Object", ImVec2(-1.0f, 34.0f))) {
            commandQueue_.enqueue(
                RemoveSceneObjectCommand{*activeObjectId}
            );
        }

        ImGui::EndChild();
    }

    void EditorPanel::drawPerformance(
        std::optional<SceneObjectId> activeObjectId
    ) {
        ImGui::BeginChild("##Performance", ImVec2(0.0f, 0.0f), true);
        drawSectionTitle("Performance");

        const float frameRate = ImGui::GetIO().Framerate;
        ImGui::Text("Frame rate");
        ImGui::SameLine();
        ImGui::TextColored(CYAN, "%.0f FPS", frameRate);
        ImGui::Text("Frame time");
        ImGui::SameLine();
        ImGui::TextColored(CYAN, "%.2f ms", frameRate > 0.0f ? 1000.0f / frameRate : 0.0f);
        ImGui::Text("Scene objects");
        ImGui::SameLine();
        ImGui::TextColored(CYAN, "%zu", scene_.getObjectCount());

        const SceneObject* object = activeObjectId
            ? scene_.findObject(*activeObjectId)
            : nullptr;
        ImGui::Text("Active points");
        ImGui::SameLine();
        ImGui::TextColored(
            CYAN,
            "%zu",
            object == nullptr ? 0 : object->getPointCloud().getPointCount()
        );
        ImGui::TextColored(MUTED, "GPU timing telemetry is not available yet.");

        ImGui::EndChild();
    }

    void EditorPanel::drawFooter(
        float width,
        float height,
        std::optional<SceneObjectId> activeObjectId
    ) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float footerY = viewport->Pos.y + height - 64.0f;

        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 16.0f, footerY));
        ImGui::SetNextWindowSize(ImVec2(width - 32.0f, 44.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 10.0f));
        ImGui::PushStyleColor(
            ImGuiCol_WindowBg,
            ImVec4(0.008f, 0.019f, 0.039f, 0.88f)
        );

        if (ImGui::Begin("##ChioniaFooter", nullptr, OVERLAY_FLAGS)) {
            const SceneObject* object = activeObjectId
                ? scene_.findObject(*activeObjectId)
                : nullptr;
            const std::size_t pointCount = object == nullptr
                ? 0
                : object->getPointCloud().getPointCount();

            ImGui::TextColored(MUTED, "Chionia Lab v0.1.0");
            ImGui::SameLine();
            ImGui::TextColored(CYAN, "|");
            ImGui::SameLine();
            ImGui::TextColored(MUTED, "Vulkan Renderer");
            ImGui::SameLine();
            ImGui::TextColored(CYAN, "|");
            ImGui::SameLine();
            ImGui::TextColored(MUTED, "%zu particles", pointCount);

            if (width > 720.0f) {
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 480.0f);
                ImGui::TextColored(
                    MUTED,
                    "Left-drag: camera  |  Right-drag: shape  |  Scroll: zoom"
                );
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

}
