#include "editor_panel.hpp"
#include "scene/ground_plane.hpp"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace chionia {

    namespace {

        constexpr ImVec4 CYAN(0.10f, 0.78f, 1.00f, 1.00f);
        constexpr ImVec4 MUTED(0.57f, 0.67f, 0.79f, 1.00f);
        constexpr const char* PRESENTATION_KEY = "lesson/presentation";
        // Exclude older subject-specific keys if they exist in a live session.
        constexpr const char* SOLAR_SYSTEM_KEY = "lesson/solar-system";
        constexpr const char* HUMAN_CELL_KEY = "lesson/biology-human-cell";
        constexpr const char* ANATOMY_KEY = "lesson/anatomy-heart";

        bool isPresentationLayerKey(const std::string& key) {
            return key == PRESENTATION_KEY ||
                key == SOLAR_SYSTEM_KEY ||
                key == HUMAN_CELL_KEY ||
                key == ANATOMY_KEY;
        }

        enum class CardiacPhase {
            Diastole,
            AtrialSystole,
            VentricularSystole,
            Relaxation
        };

        struct CardiacCycleFrame {
            CardiacPhase phase = CardiacPhase::Diastole;
            float progress = 0.0f;
            const char* label = "DIASTOLE  ·  passive ventricular filling";
        };

        CardiacCycleFrame getCardiacCycleFrame(float animationTime) {
            constexpr float CYCLE_SECONDS = 0.92f;
            const float phase = std::fmod(
                std::max(animationTime, 0.0f),
                CYCLE_SECONDS
            ) / CYCLE_SECONDS;

            if (phase < 0.50f) {
                return {CardiacPhase::Diastole, phase / 0.50f,
                    "DIASTOLE  ·  passive ventricular filling"};
            }
            if (phase < 0.60f) {
                return {CardiacPhase::AtrialSystole,
                    (phase - 0.50f) / 0.10f,
                    "ATRIAL SYSTOLE  ·  final filling"};
            }
            if (phase < 0.90f) {
                return {CardiacPhase::VentricularSystole,
                    (phase - 0.60f) / 0.30f,
                    "VENTRICULAR SYSTOLE  ·  blood ejected"};
            }
            if (phase < 1.0f) {
                return {CardiacPhase::Relaxation,
                    (phase - 0.90f) / 0.10f,
                    "ISOVOLUMETRIC RELAXATION  ·  valves reset"};
            }
            return {CardiacPhase::Diastole, 0.0f,
                "DIASTOLE  ·  passive ventricular filling"};
        }

        float ecgWaveform(float phase) {
            const auto gaussian = [](float value, float center, float width) {
                const float scaled = (value - center) / width;
                return std::exp(-0.5f * scaled * scaled);
            };
            return 0.19f * gaussian(phase, 0.245f, 0.025f) -
                0.12f * gaussian(phase, 0.315f, 0.009f) +
                0.78f * gaussian(phase, 0.332f, 0.008f) -
                0.22f * gaussian(phase, 0.351f, 0.012f) +
                0.31f * gaussian(phase, 0.735f, 0.050f);
        }

        void drawMicrophoneIcon(ImDrawList* drawList, const ImVec2& center) {
            constexpr ImU32 ICON_COLOR = IM_COL32(82, 171, 202, 190);
            drawList->AddRectFilled(
                ImVec2(center.x - 3.2f, center.y - 7.0f),
                ImVec2(center.x + 3.2f, center.y + 3.0f),
                ICON_COLOR,
                3.2f
            );
            drawList->PathArcTo(
                ImVec2(center.x, center.y + 0.5f),
                7.0f,
                0.0f,
                3.14159265358979323846f,
                16
            );
            drawList->PathStroke(ICON_COLOR, 0, 1.5f);
            drawList->AddLine(
                ImVec2(center.x, center.y + 7.5f),
                ImVec2(center.x, center.y + 11.0f),
                ICON_COLOR,
                1.5f
            );
            drawList->AddLine(
                ImVec2(center.x - 4.0f, center.y + 11.0f),
                ImVec2(center.x + 4.0f, center.y + 11.0f),
                ICON_COLOR,
                1.5f
            );
        }

        void drawCardiacCycleOverlay(
            ImDrawList* drawList,
            const ImVec2& viewportPosition,
            float width,
            float height,
            float animationTime
        ) {
            constexpr float PANEL_WIDTH = 440.0f;
            constexpr float PANEL_HEIGHT = 94.0f;
            const ImVec2 minimum(
                viewportPosition.x + width - PANEL_WIDTH - 18.0f,
                viewportPosition.y + height - PANEL_HEIGHT - 58.0f
            );
            const ImVec2 maximum(
                minimum.x + PANEL_WIDTH,
                minimum.y + PANEL_HEIGHT
            );
            const CardiacCycleFrame cycle =
                getCardiacCycleFrame(animationTime);
            const float normalizedTime = std::fmod(
                std::max(animationTime, 0.0f),
                0.92f
            ) / 0.92f;

            drawList->AddRectFilled(
                minimum,
                maximum,
                IM_COL32(2, 12, 22, 224),
                8.0f
            );
            drawList->AddRect(
                minimum,
                maximum,
                IM_COL32(39, 119, 157, 178),
                8.0f,
                0,
                1.0f
            );
            drawList->AddText(
                ImVec2(minimum.x + 12.0f, minimum.y + 8.0f),
                IM_COL32(112, 224, 255, 245),
                "ECG  /  ONE ILLUSTRATIVE HEARTBEAT"
            );
            drawList->AddText(
                ImVec2(minimum.x + 12.0f, minimum.y + 24.0f),
                IM_COL32(197, 219, 232, 235),
                cycle.label
            );

            const float plotLeft = minimum.x + 12.0f;
            const float plotRight = maximum.x - 12.0f;
            const float baseline = minimum.y + 64.0f;
            const float plotWidth = plotRight - plotLeft;
            drawList->AddLine(
                ImVec2(plotLeft, baseline),
                ImVec2(plotRight, baseline),
                IM_COL32(49, 106, 130, 125),
                1.0f
            );

            constexpr int SAMPLES = 180;
            ImVec2 previous(plotLeft, baseline);
            for (int sample = 1; sample <= SAMPLES; ++sample) {
                const float phase = static_cast<float>(sample) /
                    static_cast<float>(SAMPLES);
                const ImVec2 current(
                    plotLeft + phase * plotWidth,
                    baseline - ecgWaveform(phase) * 33.0f
                );
                drawList->AddLine(
                    previous,
                    current,
                    IM_COL32(59, 209, 243, 225),
                    1.6f
                );
                previous = current;
            }

            const float cursorX = plotLeft + normalizedTime * plotWidth;
            const float cursorY = baseline -
                ecgWaveform(normalizedTime) * 33.0f;
            drawList->AddLine(
                ImVec2(cursorX, minimum.y + 42.0f),
                ImVec2(cursorX, minimum.y + 78.0f),
                IM_COL32(255, 219, 119, 160),
                1.0f
            );
            drawList->AddCircleFilled(
                ImVec2(cursorX, cursorY),
                3.2f,
                IM_COL32(255, 225, 143, 255)
            );

            const std::array<const char*, 5> waveNames = {{"P", "Q", "R", "S", "T"}};
            const std::array<float, 5> wavePhases = {{0.245f, 0.315f, 0.332f, 0.351f, 0.735f}};
            for (std::size_t index = 0; index < waveNames.size(); ++index) {
                const float x = plotLeft + wavePhases[index] * plotWidth;
                drawList->AddText(
                    ImVec2(x - 3.0f, minimum.y + 76.0f),
                    IM_COL32(137, 178, 198, 230),
                    waveNames[index]
                );
            }
        }

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

        struct AnnotationRect {
            ImVec2 minimum;
            ImVec2 maximum;
        };

        bool overlapsWithPadding(
            const AnnotationRect& first,
            const AnnotationRect& second,
            float paddingX,
            float paddingY
        ) {
            return first.minimum.x < second.maximum.x + paddingX &&
                first.maximum.x > second.minimum.x - paddingX &&
                first.minimum.y < second.maximum.y + paddingY &&
                first.maximum.y > second.minimum.y - paddingY;
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

    const char* EditorPanel::activePresentationKey() const {
        return PRESENTATION_KEY;
    }

    bool EditorPanel::hasActivePresentation() const {
        if (activePresentationSubject_ == PresentationSubject::HumanCell) {
            return biologyLesson_.hasHumanCell();
        }
        if (activePresentationSubject_ == PresentationSubject::Anatomy) {
            return anatomyLesson_.hasHeartCycle();
        }
        if (activePresentationSubject_ == PresentationSubject::AdditionalLesson) {
            return testLesson_.hasLesson();
        }
        return activePresentationSubject_ == PresentationSubject::SolarSystem &&
            solarSystemLesson_.hasSolarSystem();
    }

    bool EditorPanel::isPresentationTabActive() const {
        return activeTab_ == Tab::Presentation;
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
        const std::function<void(const std::string&, bool)>& loadShape,
        const Camera& visualizationCamera,
        const Camera& presentationCamera,
        CameraController& visualizationCameraController,
        CameraController& presentationCameraController,
        float activeAnimationTime
    ) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 size = viewport->Size;

        const Tab previousTab = activeTab_;
        drawTopBar(size.x);
        const bool tabChanged = previousTab != activeTab_;
        const bool presentationTabActive =
            activeTab_ == Tab::Presentation;
        const Camera& activeCamera = presentationTabActive
            ? presentationCamera
            : visualizationCamera;
        CameraController& activeCameraController = presentationTabActive
            ? presentationCameraController
            : visualizationCameraController;
        const bool returnToInfinity = returnToInfinityAfterClear_;
        const std::optional<SceneObjectId> savedVisualizationObject =
            visualizationObjectBeforePresentation_;
        if (tabChanged) {
            if (activeTab_ == Tab::Presentation) {
                enterPresentationMode(
                    activeObjectId,
                    presentationCameraController,
                    size.y
                );
            }
            else if (previousTab == Tab::Presentation) {
                restorePresentationView(presentationCameraController);
            }
        }

        std::optional<SceneObjectId> displayedObjectId = activeObjectId;
        if (activeTab_ == Tab::Presentation) {
            displayedObjectId = scene_.findObjectByKey(activePresentationKey());
        }
        else if (previousTab == Tab::Presentation && tabChanged) {
            displayedObjectId = returnToInfinity
                ? scene_.findObjectByKey("procedural/infinity")
                : savedVisualizationObject;
            if (
                !displayedObjectId ||
                scene_.findObject(*displayedObjectId) == nullptr
            ) {
                displayedObjectId.reset();
                if (!returnToInfinity) {
                    for (const SceneObjectId objectId : scene_.getObjectIds()) {
                        const SceneObject* object = scene_.findObject(objectId);
                        if (
                            object != nullptr &&
                            !isPresentationLayerKey(object->getKey())
                        ) {
                            displayedObjectId = objectId;
                        }
                    }
                }
            }
        }

        drawBackground(size.x, size.y, displayedObjectId, activeCamera);
        drawSidebar(
            size.y,
            displayedObjectId,
            loadShape,
            activeCameraController
        );
        drawFooter(size.x, size.y, displayedObjectId);
        if (
            activeTab_ == Tab::Presentation &&
            hasActivePresentation()
        ) {
            drawLessonAnnotations(
                size.x,
                size.y,
                displayedObjectId,
                activeCamera,
                activeAnimationTime
            );
        }
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

        const bool presentationMode =
            activeTab_ == Tab::Presentation ||
            solarSystemLesson_.hasSolarSystem() ||
            biologyLesson_.hasHumanCell() ||
            anatomyLesson_.hasHeartCycle() ||
            testLesson_.hasLesson();
        if (!presentationMode) {
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

            const auto drawOrbit = [&](float radius, ImU32 color) {
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
    }

    void EditorPanel::drawLessonAnnotations(
        float width,
        float height,
        std::optional<SceneObjectId> activeObjectId,
        const Camera& camera,
        float animationTime
    ) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const float boardLeft = viewport->Pos.x + 306.0f;
        const float boardRight = viewport->Pos.x + width - 18.0f;
        const float boardTop = viewport->Pos.y + 140.0f;
        const float boardBottom = viewport->Pos.y + height - 52.0f;

        const bool humanCellPresentation =
            activePresentationSubject_ == PresentationSubject::HumanCell;
        const bool anatomyPresentation =
            activePresentationSubject_ == PresentationSubject::Anatomy;
        const bool additionalLessonPresentation =
            activePresentationSubject_ == PresentationSubject::AdditionalLesson;
        const char* lessonTitle = humanCellPresentation
            ? "HUMAN CELL"
            : anatomyPresentation
                ? "ANATOMY  /  THE CARDIAC CYCLE"
                : additionalLessonPresentation
                    ? testLesson_.getTitle()
                    : "SOLAR SYSTEM";
        const char* lessonSubtitle = humanCellPresentation
            ? "TYPICAL ANIMAL CELL  /  SCHEMATIC, NOT TO SCALE"
            : anatomyPresentation
                ? "FOUR CHAMBERS  /  TWO CIRCUITS  /  ANIMATED FLOW TRACER"
                : additionalLessonPresentation
                    ? testLesson_.getSubtitle()
                    : solarSystemLesson_.hasGravityExplanation()
                        ? "CYAN ARROWS  /  GRAVITY PULLS TOWARD THE SUN"
                        : "SIZES AND DISTANCES SIMPLIFIED  /  ORBITS ACCELERATED";
        drawList->AddText(
            nullptr,
            18.0f,
            ImVec2(boardLeft + 16.0f, boardTop + 6.0f),
            IM_COL32(139, 226, 255, 240),
            lessonTitle
        );
        drawList->AddText(
            nullptr,
            12.0f,
            ImVec2(boardLeft + 17.0f, boardTop + 27.0f),
            IM_COL32(118, 151, 183, 225),
            lessonSubtitle
        );
        drawList->AddLine(
            ImVec2(boardLeft + 16.0f, boardTop + 48.0f),
            ImVec2(boardRight - 12.0f, boardTop + 48.0f),
            IM_COL32(34, 116, 165, 92),
            1.0f
        );

        const glm::mat4 view = camera.getViewMatrix();
        const glm::mat4 projection = camera.getProjectionMatrix(
            width / height,
            true
        );
        glm::mat4 model(1.0f);
        float animationSpeed = 1.0f;
        if (activeObjectId) {
            const SceneObject* object = scene_.findObject(*activeObjectId);
            if (object != nullptr && isPresentationLayerKey(object->getKey())) {
                model = object->getTransform().getModelMatrix();
                animationSpeed = object->getAnimationSpeed();
            }
        }

        const ImVec2 boardCenter(
            (boardLeft + boardRight) * 0.5f,
            (boardTop + boardBottom) * 0.5f
        );
        std::vector<AnnotationRect> occupiedLabels;

        std::vector<LessonLabel> labels;
        if (humanCellPresentation) {
            const auto cellLabels = biologyLesson_.getLabels();
            labels.assign(cellLabels.begin(), cellLabels.end());
        }
        else if (anatomyPresentation) {
            const auto anatomyLabels = anatomyLesson_.getLabels();
            labels.assign(anatomyLabels.begin(), anatomyLabels.end());
        }
        else if (additionalLessonPresentation) {
            labels = testLesson_.getLabels();
        }
        else {
            const auto solarSystemLabels = solarSystemLesson_.getLabels(
                animationTime * animationSpeed
            );
            labels.assign(
                solarSystemLabels.begin(),
                solarSystemLabels.end()
            );
        }

        if (anatomyPresentation) {
            const CardiacCycleFrame cycle = getCardiacCycleFrame(
                animationTime * animationSpeed
            );
            if (
                cycle.phase == CardiacPhase::AtrialSystole ||
                cycle.phase == CardiacPhase::VentricularSystole
            ) {
                const float pulse = 0.5f + 0.5f * std::sin(
                    cycle.progress * 3.14159265359f
                );
                const bool atriaActive =
                    cycle.phase == CardiacPhase::AtrialSystole;
                const std::array<glm::vec3, 2> chamberCenters = atriaActive
                    ? std::array<glm::vec3, 2>{{
                        {-0.28f, 0.31f, 0.24f},
                        {0.27f, 0.31f, 0.24f}
                    }}
                    : std::array<glm::vec3, 2>{{
                        {-0.27f, -0.20f, 0.25f},
                        {0.27f, -0.20f, 0.27f}
                    }};
                const ImU32 pulseColor = atriaActive
                    ? IM_COL32(98, 199, 255, 165)
                    : IM_COL32(255, 153, 104, 175);

                for (const glm::vec3& center : chamberCenters) {
                    const glm::vec4 clip = projection * view * model *
                        glm::vec4(center, 1.0f);
                    if (clip.w <= 0.08f) {
                        continue;
                    }
                    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
                    const ImVec2 screen(
                        viewport->Pos.x + (ndc.x + 1.0f) * width * 0.5f,
                        viewport->Pos.y + (ndc.y + 1.0f) * height * 0.5f
                    );
                    const float radius = 24.0f + pulse * 10.0f;
                    drawList->AddCircle(
                        screen,
                        radius,
                        pulseColor,
                        40,
                        1.5f
                    );
                    drawList->AddCircle(
                        screen,
                        radius + 5.0f,
                        (pulseColor & 0x00FFFFFFu) | 0x40000000u,
                        40,
                        1.0f
                    );
                }

                const auto drawImpulse = [&](const glm::vec3& worldPosition) {
                    const glm::vec4 clip = projection * view * model *
                        glm::vec4(worldPosition, 1.0f);
                    if (clip.w <= 0.08f) {
                        return;
                    }
                    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
                    const ImVec2 screen(
                        viewport->Pos.x + (ndc.x + 1.0f) * width * 0.5f,
                        viewport->Pos.y + (ndc.y + 1.0f) * height * 0.5f
                    );
                    drawList->AddCircleFilled(
                        screen,
                        7.0f,
                        IM_COL32(255, 226, 115, 82)
                    );
                    drawList->AddCircleFilled(
                        screen,
                        3.0f,
                        IM_COL32(255, 246, 185, 255)
                    );
                };

                if (atriaActive) {
                    const glm::vec3 sinoatrialNode(-0.36f, 0.45f, 0.255f);
                    const glm::vec3 atrioventricularNode(-0.01f, 0.08f, 0.26f);
                    drawImpulse(glm::mix(
                        sinoatrialNode,
                        atrioventricularNode,
                        cycle.progress
                    ));
                }
                else {
                    constexpr glm::vec3 BUNDLE_START(-0.01f, 0.08f, 0.26f);
                    constexpr glm::vec3 BUNDLE_SPLIT(-0.01f, -0.32f, 0.26f);
                    const float branchProgress = std::clamp(
                        (cycle.progress - 0.20f) / 0.80f,
                        0.0f,
                        1.0f
                    );
                    if (cycle.progress < 0.20f) {
                        drawImpulse(glm::mix(
                            BUNDLE_START,
                            BUNDLE_SPLIT,
                            cycle.progress / 0.20f
                        ));
                    }
                    else {
                        drawImpulse(glm::mix(
                            BUNDLE_SPLIT,
                            glm::vec3(-0.31f, -0.66f, 0.26f),
                            branchProgress
                        ));
                        drawImpulse(glm::mix(
                            BUNDLE_SPLIT,
                            glm::vec3(0.31f, -0.66f, 0.26f),
                            branchProgress
                        ));
                    }
                }
            }
        }

        if (additionalLessonPresentation) {
            const auto marker = testLesson_.getAnimationMarker(
                animationTime * animationSpeed
            );
            if (marker) {
                const glm::vec4 clip = projection * view * model *
                    glm::vec4(marker->position, 1.0f);
                if (clip.w > 0.08f) {
                    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
                    const ImVec2 screen(
                        viewport->Pos.x + (ndc.x + 1.0f) * width * 0.5f,
                        viewport->Pos.y + (ndc.y + 1.0f) * height * 0.5f
                    );
                    const float pulse = 0.5f + 0.5f * std::sin(
                        animationTime * 4.2f
                    );
                    const ImU32 markerColor = ImGui::ColorConvertFloat4ToU32(
                        ImVec4(
                            marker->color.r,
                            marker->color.g,
                            marker->color.b,
                            marker->color.a
                        )
                    );
                    drawList->AddCircle(
                        screen,
                        5.0f + pulse * 2.0f,
                        markerColor,
                        24,
                        2.0f
                    );
                    drawList->AddCircleFilled(
                        screen,
                        2.3f,
                        markerColor
                    );
                }
            }
        }

        if (anatomyPresentation) {
            // This schematic tracer uses a slower timescale than the ECG: a
            // single red blood cell traverses the pulmonary and systemic
            // circuits over several seconds, not within one heartbeat.
            constexpr std::array<glm::vec3, 12> FLOW_ROUTE = {{
                {-0.25f, 1.00f, 0.08f},
                {-0.28f, 0.31f, 0.24f},
                {-0.27f, 0.035f, 0.25f},
                {-0.27f, -0.20f, 0.25f},
                {-0.12f, 0.65f, 0.18f},
                {-0.70f, 0.84f, 0.09f},
                {-0.70f, 0.56f, 0.07f},
                {0.27f, 0.31f, 0.24f},
                {0.27f, 0.035f, 0.25f},
                {0.27f, -0.20f, 0.27f},
                {0.19f, 0.73f, 0.17f},
                {-0.22f, 1.07f, 0.10f}
            }};
            constexpr float TRACER_SECONDS = 12.0f;
            const float tracerPosition = std::fmod(
                std::max(animationTime, 0.0f),
                TRACER_SECONDS
            ) / TRACER_SECONDS * static_cast<float>(FLOW_ROUTE.size() - 1);
            const std::size_t routeSegment = std::min(
                static_cast<std::size_t>(tracerPosition),
                FLOW_ROUTE.size() - 2
            );
            const float segmentProgress = tracerPosition -
                static_cast<float>(routeSegment);
            const glm::vec3 tracerWorldPosition = glm::mix(
                FLOW_ROUTE[routeSegment],
                FLOW_ROUTE[routeSegment + 1],
                segmentProgress
            );
            const glm::vec4 tracerClip = projection * view * model *
                glm::vec4(tracerWorldPosition, 1.0f);
            if (tracerClip.w > 0.08f) {
                const glm::vec3 tracerNdc = glm::vec3(tracerClip) /
                    tracerClip.w;
                const ImVec2 tracerScreen(
                    viewport->Pos.x + (tracerNdc.x + 1.0f) * width * 0.5f,
                    viewport->Pos.y + (tracerNdc.y + 1.0f) * height * 0.5f
                );
                const bool oxygenated = routeSegment >= 5;
                const ImU32 tracerColor = oxygenated
                    ? IM_COL32(255, 94, 111, 255)
                    : IM_COL32(68, 192, 255, 255);
                drawList->AddCircleFilled(
                    tracerScreen,
                    9.0f,
                    oxygenated
                        ? IM_COL32(255, 94, 111, 70)
                        : IM_COL32(68, 192, 255, 70)
                );
                drawList->AddCircle(
                    tracerScreen,
                    5.5f,
                    tracerColor,
                    20,
                    2.0f
                );
                drawList->AddCircleFilled(
                    tracerScreen,
                    2.0f,
                    IM_COL32(255, 245, 221, 255)
                );
            }
        }

        for (const LessonLabel& label : labels) {
            if (presentationLabelMode_ == PresentationLabelMode::Hidden) {
                continue;
            }
            if (
                presentationLabelMode_ == PresentationLabelMode::SystemObjects &&
                label.text == "Moon"
            ) {
                continue;
            }

            const glm::vec4 clipPosition =
                projection * view * model * glm::vec4(label.position, 1.0f);
            if (clipPosition.w <= 0.08f) {
                continue;
            }

            const glm::vec3 ndc = glm::vec3(clipPosition) / clipPosition.w;
            if (
                ndc.x < -1.1f || ndc.x > 1.1f ||
                ndc.y < -1.1f || ndc.y > 1.1f
            ) {
                continue;
            }

            const ImVec2 markerPosition(
                viewport->Pos.x + (ndc.x + 1.0f) * width * 0.5f,
                viewport->Pos.y + (ndc.y + 1.0f) * height * 0.5f
            );
            const ImVec2 textSize = ImGui::CalcTextSize(
                label.text.data(),
                label.text.data() + label.text.size()
            );
            const ImVec2 labelSize(textSize.x + 10.0f, 19.0f);

            float baseAngle = std::atan2(
                markerPosition.y - boardCenter.y,
                markerPosition.x - boardCenter.x
            );
            if (label.text == "Sun") {
                baseAngle = -1.57079632679f;
            }

            AnnotationRect chosenRect{};
            bool foundPlacement = false;
            bool hasCandidate = false;
            int bestOverlapCount = 1000000;
            for (const float distance : {17.0f, 24.0f, 31.0f, 38.0f, 45.0f}) {
                for (int direction = 0; direction < 12; ++direction) {
                    const float angle = baseAngle +
                        static_cast<float>(direction) * 0.52359877559f;
                    const ImVec2 center(
                        markerPosition.x + std::cos(angle) * distance,
                        markerPosition.y + std::sin(angle) * distance
                    );
                    const AnnotationRect candidate{
                        ImVec2(
                            center.x - labelSize.x * 0.5f,
                            center.y - labelSize.y * 0.5f
                        ),
                        ImVec2(
                            center.x + labelSize.x * 0.5f,
                            center.y + labelSize.y * 0.5f
                        )
                    };

                    if (
                        candidate.minimum.x < boardLeft + 8.0f ||
                        candidate.maximum.x > boardRight - 8.0f ||
                        candidate.minimum.y < boardTop + 54.0f ||
                        candidate.maximum.y > boardBottom - 4.0f
                    ) {
                        continue;
                    }
                    hasCandidate = true;

                    int overlapCount = 0;
                    for (const AnnotationRect& occupied : occupiedLabels) {
                        if (overlapsWithPadding(candidate, occupied, 3.0f, 2.0f)) {
                            ++overlapCount;
                        }
                    }
                    if (overlapCount < bestOverlapCount) {
                        chosenRect = candidate;
                        bestOverlapCount = overlapCount;
                    }
                    if (overlapCount == 0) {
                        foundPlacement = true;
                        break;
                    }
                }
                if (foundPlacement) {
                    break;
                }
            }
            if (!hasCandidate) {
                const ImVec2 center(
                    std::clamp(
                        markerPosition.x,
                        boardLeft + 8.0f + labelSize.x * 0.5f,
                        boardRight - 8.0f - labelSize.x * 0.5f
                    ),
                    std::clamp(
                        markerPosition.y,
                        boardTop + 54.0f + labelSize.y * 0.5f,
                        boardBottom - 4.0f - labelSize.y * 0.5f
                    )
                );
                chosenRect = {
                    ImVec2(
                        center.x - labelSize.x * 0.5f,
                        center.y - labelSize.y * 0.5f
                    ),
                    ImVec2(
                        center.x + labelSize.x * 0.5f,
                        center.y + labelSize.y * 0.5f
                    )
                };
            }

            const ImVec4 labelTint(
                label.color.r,
                label.color.g,
                label.color.b,
                0.50f
            );
            const ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(labelTint);
            drawList->AddCircleFilled(
                markerPosition,
                1.5f,
                IM_COL32(125, 173, 196, 138)
            );

            occupiedLabels.push_back(chosenRect);
            drawList->AddRectFilled(
                chosenRect.minimum,
                chosenRect.maximum,
                IM_COL32(3, 12, 24, 166),
                4.0f
            );
            drawList->AddRect(
                chosenRect.minimum,
                chosenRect.maximum,
                borderColor,
                4.0f,
                ImDrawFlags_None,
                0.8f
            );
            drawList->AddText(
                ImVec2(chosenRect.minimum.x + 5.0f, chosenRect.minimum.y + 2.0f),
                IM_COL32(201, 216, 230, 222),
                label.text.data(),
                label.text.data() + label.text.size()
            );
        }

        if (anatomyPresentation) {
            drawCardiacCycleOverlay(
                drawList,
                viewport->Pos,
                width,
                height,
                animationTime * animationSpeed
            );
        }
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
                "Presentation",
                activeTab_ == Tab::Presentation,
                ImVec2(126.0f, 44.0f)
            )) {
                activeTab_ = Tab::Presentation;
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

    void EditorPanel::enterPresentationMode(
        std::optional<SceneObjectId> activeObjectId,
        CameraController& cameraController,
        float viewportHeight
    ) {
        const char* presentationKey = activePresentationKey();
        if (activeObjectId) {
            const SceneObject* activeObject = scene_.findObject(*activeObjectId);
            if (
                activeObject != nullptr &&
                !isPresentationLayerKey(activeObject->getKey())
            ) {
                visualizationObjectBeforePresentation_ = *activeObjectId;
            }
        }

        if (!settingsBeforeLesson_) {
            settingsBeforeLesson_ = scene_.getRenderSettings();
        }

        if (!presentationRenderSettings_) {
            SceneRenderSettings settings = scene_.getRenderSettings();
            settings.morphDurationSeconds = 2.6f;
            settings.vibrationEnabled = false;
            settings.morphingEnabled = true;
            settings.autoMorphEnabled = false;
            settings.groundReflectionEnabled = false;
            settings.energyEffectsEnabled = true;
            presentationRenderSettings_ = settings;
        }
        commandQueue_.enqueue(
            SetSceneRenderSettingsCommand{*presentationRenderSettings_}
        );

        if (!presentationCameraInitialized_) {
            cameraController.setOrbitTarget(glm::vec3(0.0f));
            cameraController.setOrbitAngles(
                glm::vec2(glm::radians(180.0f), 0.0f)
            );
            cameraController.setOrbitRadius(3.3f);
            cameraController.shiftOrbitTargetHorizontally(
                144.0f,
                viewportHeight
            );
            presentationCameraInitialized_ = true;
        }
        cameraController.resetInputState();

        if (const auto presentationId = scene_.findObjectByKey(presentationKey)) {
            commandQueue_.enqueue(
                SelectSceneObjectCommand{*presentationId, true}
            );
        }
        else if (
            activePresentationSubject_ == PresentationSubject::HumanCell &&
            biologyLesson_.hasHumanCell()
        ) {
            commandQueue_.enqueue(
                UpsertPointCloudCommand{
                    presentationKey,
                    biologyLesson_.buildPointCloud(),
                    true
                }
            );
        }
        else if (
            activePresentationSubject_ == PresentationSubject::Anatomy &&
            anatomyLesson_.hasHeartCycle()
        ) {
            commandQueue_.enqueue(
                UpsertPointCloudCommand{
                    presentationKey,
                    anatomyLesson_.buildPointCloud(),
                    true
                }
            );
        }
        else if (
            activePresentationSubject_ == PresentationSubject::SolarSystem &&
            solarSystemLesson_.hasSolarSystem()
        ) {
            commandQueue_.enqueue(
                UpsertPointCloudCommand{
                    presentationKey,
                    solarSystemLesson_.buildPointCloud(),
                    true
                }
            );
        }
        else if (
            activePresentationSubject_ == PresentationSubject::AdditionalLesson &&
            testLesson_.hasLesson()
        ) {
            commandQueue_.enqueue(
                UpsertPointCloudCommand{
                    presentationKey,
                    testLesson_.buildPointCloud(),
                    true
                }
            );
        }
        else {
            InfinityGeneratorConfig presentationInfinityConfig = infinityConfig_;
            presentationInfinityConfig.pointCount = MORPH_PARTICLE_COUNT;
            commandQueue_.enqueue(
                UpsertPointCloudCommand{
                    presentationKey,
                    InfinityGenerator::generate(presentationInfinityConfig),
                    true
                }
            );
            lessonFeedback_ =
                "Infinity is ready. Apply spoken text to build the presentation.";
        }

        cameraController.setCinematicOrbit(
            cinematicCameraEnabled_ && hasActivePresentation()
        );
    }

    void EditorPanel::restorePresentationView(
        CameraController& cameraController,
        bool selectVisualizationObject
    ) {
        cameraController.setCinematicOrbit(false);
        cameraController.resetInputState();
        if (selectVisualizationObject) {
            presentationRenderSettings_ = scene_.getRenderSettings();
        }
        else {
            presentationRenderSettings_.reset();
        }
        if (settingsBeforeLesson_) {
            commandQueue_.enqueue(
                SetSceneRenderSettingsCommand{*settingsBeforeLesson_}
            );
            settingsBeforeLesson_.reset();
        }

        if (selectVisualizationObject) {
            std::optional<SceneObjectId> visualizationObjectId;
            if (returnToInfinityAfterClear_) {
                visualizationObjectId =
                    scene_.findObjectByKey("procedural/infinity");
            }
            else {
                visualizationObjectId =
                    visualizationObjectBeforePresentation_;
            }
            if (
                !visualizationObjectId ||
                scene_.findObject(*visualizationObjectId) == nullptr
            ) {
                visualizationObjectId.reset();
                if (!returnToInfinityAfterClear_) {
                    for (const SceneObjectId objectId : scene_.getObjectIds()) {
                        const SceneObject* object = scene_.findObject(objectId);
                        if (
                            object != nullptr &&
                            !isPresentationLayerKey(object->getKey())
                        ) {
                            visualizationObjectId = objectId;
                        }
                    }
                }
            }

            if (visualizationObjectId) {
                commandQueue_.enqueue(
                    SelectSceneObjectCommand{*visualizationObjectId, true}
                );
            }
        }
        visualizationObjectBeforePresentation_.reset();
        if (selectVisualizationObject) {
            returnToInfinityAfterClear_ = false;
        }
    }

    void EditorPanel::drawSidebar(
        float height,
        std::optional<SceneObjectId> activeObjectId,
        const std::function<void(const std::string&, bool)>& loadShape,
        CameraController& cameraController
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
            if (activeTab_ == Tab::Visualization) {
                const float libraryHeight = panelHeight * 0.44f;
                drawShapeLibrary(libraryHeight, activeObjectId, loadShape);
            }

            if (activeTab_ == Tab::Visualization) {
                ImGui::Spacing();
                drawVisualizationControls();
            }
            else if (activeTab_ == Tab::Presentation) {
                drawPresentationControls(cameraController);
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
        const std::function<void(const std::string&, bool)>& loadShape
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
        const std::size_t visibleCount = count;
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
                loadShape(path, shapeOutlinesEnabled_);
            }

            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }

        ImGui::EndChild();

        const SceneObject* selectedObject = activeObjectId
            ? scene_.findObject(*activeObjectId)
            : nullptr;
        const bool outlineSupported = selectedObject != nullptr && (
            containsAsset(selectedObject->getKey(), "shape/cube.json") ||
            containsAsset(selectedObject->getKey(), "shape/pyramid.json") ||
            containsAsset(selectedObject->getKey(), "shape/cone.json")
        );

        ImGui::BeginDisabled(!outlineSupported);
        const bool outlinesChanged = ImGui::Checkbox(
            "Point outlines",
            &shapeOutlinesEnabled_
        );
        ImGui::EndDisabled();

        if (outlineSupported && outlinesChanged && loadShape) {
            loadShape(selectedObject->getKey(), shapeOutlinesEnabled_);
        }
        if (outlineSupported) {
            ImGui::TextWrapped("Bright edge points for cube, pyramid, and cone.");
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, MUTED);
            ImGui::TextWrapped("Choose a cube, pyramid, or cone for edge detail.");
            ImGui::PopStyleColor();
        }
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

        ImGui::Spacing();
        drawProceduralControls();

        ImGui::EndChild();
    }

    void EditorPanel::drawProceduralControls() {
        ImGui::Separator();
        if (!ImGui::CollapsingHeader("Infinity Generator")) {
            return;
        }
        ImGui::TextWrapped(
            "Create or refine the infinity shape in the Visualization scene."
        );

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
    }

    void EditorPanel::drawPresentationControls(CameraController& cameraController) {
        ImGui::BeginChild("##LessonControls", ImVec2(0.0f, 0.0f), true);
        drawSectionTitle("Build from lesson text");
        ImGui::TextWrapped(
            "Enter a lesson idea or paste a supported passage to create a "
            "visual explanation."
        );
        ImGui::Spacing();
        ImGui::TextColored(MUTED, "Start with an example");
        constexpr std::array<const char*, 11> TEST_EXAMPLES = {{
            "Choose an example...",
            "Solar system  /  astronomy",
            "Human cell  /  biology",
            "Heart and cardiac cycle  /  anatomy",
            "Series circuit  /  physics",
            "Carbon-12 atom  /  chemistry",
            "Water cycle  /  earth science",
            "Plate boundaries  /  geology",
            "Marine food web  /  ecology",
            "Pythagorean theorem  /  mathematics",
            "Cold front  /  meteorology"
        }};
        constexpr std::array<const char*, 11> TEST_TRANSCRIPTS = {{
            "",
            "Our solar system consists of the Sun and eight planets that orbit "
                "it. Their paths are shown as near-circular for this model. "
                "Distances, sizes, and orbital speeds are simplified.",
            "A typical human cell is enclosed by a plasma membrane. Its "
            "cytoplasm contains organelles, and its nucleus stores DNA as "
            "chromatin. Ribosomes build proteins, while mitochondria "
                "produce ATP.",
            "Today, we will follow one red blood cell through the heart and "
                "lungs. Deoxygenated blood returning from the body enters the "
                "right atrium through the venae cavae, passes through the "
                "tricuspid valve into the right ventricle, and is pumped "
                "through the pulmonary valve and pulmonary arteries to the "
                "lungs. There, carbon dioxide leaves the blood and oxygen "
                "enters. Oxygenated blood returns through the pulmonary veins "
                "to the left atrium, crosses the mitral valve into the left "
                "ventricle, and is ejected through the aortic valve into the "
                "aorta and systemic circulation. The four valves keep blood "
                "moving one way. The animation is schematic and not to scale.",
            "In a series circuit, a battery, resistor, and lamp are connected "
                "in one closed path. Conventional current leaves the battery "
                "positive terminal and passes through the resistor and lamp "
                "before returning to the negative terminal. Electrons drift "
                "the opposite way in the external wires. The current is the "
                "same through every component in this single series loop.",
            "A carbon-12 atom has six protons and six neutrons in its nucleus, "
                "with six electrons. In this simplified shell model, two "
                "electrons occupy the first shell and four are shown in the "
                "second. These rings are a teaching diagram: real electrons "
                "occupy quantum states and are not tiny planets on fixed "
                "circular tracks.",
            "In the water cycle, sunlight supplies energy for evaporation "
                "from oceans and lakes, while plants return water vapor by "
                "transpiration. As moist air cools, condensation forms clouds; "
                "water then falls as precipitation. Some flows over land as "
                "runoff, and some infiltrates soil to recharge groundwater. "
                "Rivers and groundwater eventually carry water back toward "
                "the ocean. The cycle is continuous and has no single start.",
            "At a divergent plate boundary, plates move apart and magma can "
                "rise and cool to form new crust. At an oceanic-continental "
                "convergent boundary, denser oceanic lithosphere bends "
                "beneath the continent in a subduction zone. At a transform "
                "boundary, neighboring plates slide sideways past one "
                "another. These cross-sections are simplified and not to "
                "scale.",
            "In this simplified marine food web, sunlight supplies energy to "
                "phytoplankton, the producers. Zooplankton eat phytoplankton, "
                "and small fish eat zooplankton; larger fish may eat the "
                "smaller fish. Arrows point from food to consumer. Decomposers "
                "break down remains and return nutrients to the ecosystem, "
                "while energy flows through the web and is eventually lost "
                "as heat.",
            "For a right triangle, the Pythagorean theorem states that a "
                "squared plus b squared equals c squared, where c is the "
                "hypotenuse opposite the right angle. In a 3-4-5 example, "
                "the squares on the legs have areas 9 and 16 square units, "
                "and the square on the hypotenuse has area 25 square units: "
                "9 plus 16 equals 25.",
            "A cold front forms when advancing, denser cold air wedges beneath "
                "warmer air and forces it upward. If the warm air is moist and "
                "the atmosphere is unstable, rapid uplift may produce clouds "
                "and showers or thunderstorms near the front. The exact "
                "weather depends on local conditions; this cross-section is "
                "a teaching schematic, not a forecast."
        }};
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo(
                "##TestExample",
                &testLessonExampleIndex_,
                TEST_EXAMPLES.data(),
                static_cast<int>(TEST_EXAMPLES.size())
            )) {
            if (testLessonExampleIndex_ > 0) {
                std::snprintf(
                    lessonTranscript_.data(),
                    lessonTranscript_.size(),
                    "%s",
                    TEST_TRANSCRIPTS[static_cast<std::size_t>(
                        testLessonExampleIndex_
                    )]
                );
            }
        }
        ImGui::TextColored(CYAN, "TRANSCRIPT");
        ImGui::SameLine();
        const std::size_t transcriptBytes =
            std::char_traits<char>::length(lessonTranscript_.data());
        std::size_t transcriptLength = 0;
        for (std::size_t i = 0; i < transcriptBytes; ++i) {
            const unsigned char character =
                static_cast<unsigned char>(lessonTranscript_[i]);
            if ((character & 0xC0u) != 0x80u) {
                ++transcriptLength;
            }
        }
        ImGui::TextColored(
            MUTED,
            "%zu / %zu characters",
            transcriptLength,
            lessonTranscript_.size() - 1
        );
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(
            ImGuiCol_FrameBg,
            ImVec4(0.012f, 0.035f, 0.061f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_FrameBgHovered,
            ImVec4(0.018f, 0.055f, 0.087f, 1.0f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_Border,
            ImVec4(0.04f, 0.37f, 0.58f, 0.82f)
        );
        ImGui::InputTextMultiline(
            "##LessonTranscript",
            lessonTranscript_.data(),
            lessonTranscript_.size(),
            ImVec2(-1.0f, 148.0f),
            ImGuiInputTextFlags_WordWrap
        );
        const ImVec2 transcriptMinimum = ImGui::GetItemRectMin();
        const ImVec2 transcriptMaximum = ImGui::GetItemRectMax();
        const ImVec2 cursorAfterTranscript = ImGui::GetCursorScreenPos();
        ImDrawList* promptDrawList = ImGui::GetWindowDrawList();
        if (lessonTranscript_[0] == '\0' && !ImGui::IsItemActive()) {
            promptDrawList->AddText(
                ImVec2(transcriptMinimum.x + 11.0f, transcriptMinimum.y + 11.0f),
                IM_COL32(113, 197, 222, 225),
                "What would you like to teach?"
            );
            promptDrawList->AddText(
                ImVec2(transcriptMinimum.x + 11.0f, transcriptMinimum.y + 33.0f),
                IM_COL32(129, 155, 177, 205),
                "Enter a topic or paste a lesson passage."
            );
        }

        const ImVec2 microphoneButtonSize(34.0f, 34.0f);
        const ImVec2 microphoneButtonPosition(
            transcriptMaximum.x - microphoneButtonSize.x - 7.0f,
            transcriptMaximum.y - microphoneButtonSize.y - 7.0f
        );
        ImGui::SetCursorScreenPos(microphoneButtonPosition);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.018f, 0.060f, 0.092f, 0.96f)
        );
        ImGui::PushStyleColor(
            ImGuiCol_Border,
            ImVec4(0.04f, 0.37f, 0.58f, 0.65f)
        );
        ImGui::BeginDisabled();
        ImGui::Button("##LessonMicrophone", microphoneButtonSize);
        const bool microphoneHovered = ImGui::IsItemHovered(
            ImGuiHoveredFlags_AllowWhenDisabled
        );
        const ImVec2 microphoneButtonMinimum = ImGui::GetItemRectMin();
        ImGui::EndDisabled();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        drawMicrophoneIcon(
            ImGui::GetWindowDrawList(),
            ImVec2(
                microphoneButtonMinimum.x + microphoneButtonSize.x * 0.5f,
                microphoneButtonPosition.y + microphoneButtonSize.y * 0.5f - 1.0f
            )
        );
        if (microphoneHovered) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Microphone input is not available yet.");
            ImGui::EndTooltip();
        }
        ImGui::SetCursorScreenPos(cursorAfterTranscript);
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if (ImGui::Button("Build presentation", ImVec2(-1.0f, 36.0f))) {
            const std::string_view transcript(lessonTranscript_.data());
            const bool anatomyRecognized =
                anatomyLesson_.recognizesTranscript(transcript);
            const bool cellRecognized =
                biologyLesson_.recognizesTranscript(transcript);
            const bool solarSystemRecognized =
                solarSystemLesson_.recognizesTranscript(transcript);
            const bool additionalLessonRecognized =
                testLesson_.recognizesTranscript(transcript);
            const bool anatomyChanged =
                anatomyLesson_.applyTranscript(transcript);
            const bool cellChanged = biologyLesson_.applyTranscript(transcript);
            const bool solarSystemChanged =
                solarSystemLesson_.applyTranscript(transcript);
            const bool additionalLessonChanged =
                testLesson_.applyTranscript(transcript);
            const PresentationSubject previousSubject =
                activePresentationSubject_;
            if (anatomyRecognized) {
                activePresentationSubject_ = PresentationSubject::Anatomy;
            }
            else if (cellRecognized) {
                activePresentationSubject_ = PresentationSubject::HumanCell;
            }
            else if (additionalLessonRecognized) {
                activePresentationSubject_ =
                    PresentationSubject::AdditionalLesson;
            }
            else if (solarSystemRecognized) {
                activePresentationSubject_ = PresentationSubject::SolarSystem;
            }

            const bool subjectChanged =
                activePresentationSubject_ != previousSubject;
            const bool recognized =
                anatomyRecognized || cellRecognized ||
                additionalLessonRecognized || solarSystemRecognized;
            if (
                recognized &&
                (anatomyChanged || cellChanged || solarSystemChanged ||
                 additionalLessonChanged || subjectChanged)
            ) {
                SceneRenderSettings settings = scene_.getRenderSettings();
                settings.vibrationEnabled = false;
                settings.morphingEnabled = true;
                settings.autoMorphEnabled = false;
                settings.groundReflectionEnabled = false;
                settings.energyEffectsEnabled = false;
                presentationRenderSettings_ = settings;
                commandQueue_.enqueue(
                    SetSceneRenderSettingsCommand{settings}
                );
                if (
                    activePresentationSubject_ ==
                    PresentationSubject::Anatomy
                ) {
                    if (anatomyChanged || subjectChanged) {
                        // The default orbit sees the posterior side of this
                        // anterior cutaway, reversing patient left and right.
                        cameraController.setOrbitAngles(glm::vec2(0.0f));
                        cameraController.setOrbitTarget(glm::vec3(0.0f));
                        cameraController.setOrbitRadius(4.0f);
                    }
                    commandQueue_.enqueue(
                        UpsertPointCloudCommand{
                            PRESENTATION_KEY,
                            anatomyLesson_.buildPointCloud()
                        }
                    );
                    lessonFeedback_ =
                        "Anatomy lecture built: follow blood through the "
                        "four chambers, lungs, and systemic circuit.";
                }
                else if (
                    activePresentationSubject_ ==
                    PresentationSubject::HumanCell
                ) {
                    commandQueue_.enqueue(
                        UpsertPointCloudCommand{
                            PRESENTATION_KEY,
                            biologyLesson_.buildPointCloud()
                        }
                    );
                    lessonFeedback_ =
                        "Human cell built: membrane, cytoplasm, nucleus, DNA "
                        "and organelles.";
                }
                else if (
                    activePresentationSubject_ ==
                    PresentationSubject::AdditionalLesson
                ) {
                    if (additionalLessonChanged || subjectChanged) {
                        // These teaching diagrams are drawn face-on in the XY
                        // plane, so use a consistent front view when switching.
                        cameraController.setOrbitAngles(glm::vec2(0.0f));
                        cameraController.setOrbitTarget(glm::vec3(0.0f));
                        cameraController.setOrbitRadius(4.2f);
                    }
                    commandQueue_.enqueue(
                        UpsertPointCloudCommand{
                            PRESENTATION_KEY,
                            testLesson_.buildPointCloud()
                        }
                    );
                    lessonFeedback_ = testLesson_.getFeedback();
                }
                else {
                    commandQueue_.enqueue(
                        UpsertPointCloudCommand{
                            PRESENTATION_KEY,
                            solarSystemLesson_.buildPointCloud()
                        }
                    );
                    lessonFeedback_ =
                        solarSystemLesson_.hasGravityExplanation()
                            ? "Gravity added to the existing solar-system scene."
                            : "Solar-system presentation built.";
                }
                cameraController.setCinematicOrbit(
                    cinematicCameraEnabled_ && hasActivePresentation()
                );
            }
            else if (recognized) {
                lessonFeedback_ =
                    "That lesson is already in the presentation; the scene is unchanged.";
            }
            else {
                lessonFeedback_ =
                    "Waiting for one of the supported test lessons. "
                    "Choose an example or paste a matching passage.";
            }
        }

        ImGui::Spacing();
        ImGui::TextWrapped("%s", lessonFeedback_.c_str());

        if (ImGui::Checkbox("Cinematic camera", &cinematicCameraEnabled_)) {
            cameraController.setCinematicOrbit(
                cinematicCameraEnabled_ && hasActivePresentation()
            );
        }
        ImGui::TextColored(MUTED, "Slow orbit pauses while you drag or zoom.");

        if (hasActivePresentation()) {
            ImGui::Spacing();
            ImGui::Separator();
            const char* labelModes[] = {
                "System objects",
                "All objects",
                "Hidden"
            };
            int labelMode = static_cast<int>(presentationLabelMode_);
            ImGui::TextColored(CYAN, "Labels");
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::Combo("##PresentationLabelMode", &labelMode, labelModes, 3)) {
                presentationLabelMode_ =
                    static_cast<PresentationLabelMode>(labelMode);
            }
            if (
                activePresentationSubject_ == PresentationSubject::HumanCell &&
                presentationLabelMode_ == PresentationLabelMode::SystemObjects
            ) {
                ImGui::TextColored(
                    MUTED,
                    "Cell membrane, cytoplasm, nucleus, chromatin, "
                    "mitochondria, ER, ribosomes and Golgi."
                );
            }
            else if (
                activePresentationSubject_ == PresentationSubject::Anatomy &&
                presentationLabelMode_ == PresentationLabelMode::SystemObjects
            ) {
                ImGui::TextColored(
                    MUTED,
                    "Four chambers, four valves, great vessels, lungs, and "
                    "the cardiac conduction pathway."
                );
            }
            else if (
                activePresentationSubject_ == PresentationSubject::SolarSystem &&
                presentationLabelMode_ == PresentationLabelMode::SystemObjects
            ) {
                ImGui::TextColored(
                    MUTED,
                    "Sun, eight planets, Pluto the dwarf planet; Moon tucked away."
                );
            }
            else if (
                activePresentationSubject_ == PresentationSubject::AdditionalLesson &&
                presentationLabelMode_ == PresentationLabelMode::SystemObjects
            ) {
                ImGui::TextColored(
                    MUTED,
                    "Labels identify the main parts and processes in this diagram."
                );
            }

            if (
                activePresentationSubject_ == PresentationSubject::HumanCell
            ) {
                ImGui::Spacing();
                ImGui::TextColored(CYAN, "Scene contents");
                ImGui::TextWrapped(
                    "A simplified animal cell with one plasma-membrane outline, "
                    "cytoplasm, a nucleus with chromatin and nucleolus, two "
                    "mitochondria, folded endoplasmic reticulum, ribosomes and "
                    "a compact Golgi stack. Vesicles move slowly nearby."
                );
            }
            else if (
                activePresentationSubject_ == PresentationSubject::Anatomy
            ) {
                ImGui::Spacing();
                ImGui::TextColored(CYAN, "Scene contents");
                ImGui::TextWrapped(
                    "Anterior heart cutaway: right and left atria, right and "
                    "left ventricles, the four one-way valves, venae cavae, "
                    "pulmonary vessels, lungs, aortic arch, and conduction "
                    "pathway."
                );
                ImGui::TextColored(CYAN, "Blood-flow sequence");
                ImGui::TextWrapped(
                    "Body → venae cavae → right atrium → tricuspid valve → "
                    "right ventricle → pulmonary arteries → lungs → pulmonary "
                    "veins → left atrium → mitral valve → left ventricle → "
                    "aortic valve → aorta → body."
                );
                ImGui::TextWrapped(
                    "Blue: lower-oxygen blood. Red: oxygenated blood. The "
                    "ECG and chamber highlights illustrate diastole, atrial "
                    "systole, ventricular ejection, and relaxation. Schematic, "
                    "not to scale."
                );
            }
            else if (
                activePresentationSubject_ == PresentationSubject::AdditionalLesson
            ) {
                ImGui::Spacing();
                ImGui::TextColored(CYAN, "Scene contents");
                ImGui::TextWrapped("%s", testLesson_.getDescription());
            }
            else {
                ImGui::Spacing();
                ImGui::TextColored(CYAN, "Scene contents");
                ImGui::TextWrapped(
                    "Sun, eight planets, one example moon, and Pluto, a dwarf "
                    "planet. Sizes, distances, and orbit speeds are illustrative."
                );
                ImGui::TextColored(CYAN, "Planets");
                ImGui::TextWrapped(
                    "Mercury  Venus  Earth  Mars  Jupiter  Saturn  Uranus  Neptune"
                );

                ImGui::Spacing();
                if (solarSystemLesson_.hasGravityExplanation()) {
                    ImGui::TextWrapped(
                        "Gravity layer: arrows point inward toward the Sun. "
                        "The solar system remains visible underneath."
                    );
                }
                else {
                    ImGui::TextColored(
                        MUTED,
                        "Waiting for the gravity explanation."
                    );
                }
            }
        }

        ImGui::Spacing();
        if (ImGui::Button("Clear presentation", ImVec2(-1.0f, 34.0f))) {
            solarSystemLesson_.clear();
            biologyLesson_.clear();
            anatomyLesson_.clear();
            testLesson_.clear();
            activePresentationSubject_ = PresentationSubject::None;
            testLessonExampleIndex_ = 0;
            lessonTranscript_.fill('\0');
            for (const char* layerKey : {
                PRESENTATION_KEY,
                SOLAR_SYSTEM_KEY,
                HUMAN_CELL_KEY,
                ANATOMY_KEY
            }) {
                if (const auto objectId = scene_.findObjectByKey(layerKey)) {
                    commandQueue_.enqueue(RemoveSceneObjectCommand{*objectId});
                }
            }

            if (const auto infinityObjectId =
                    scene_.findObjectByKey("procedural/infinity")) {
                commandQueue_.enqueue(
                    SelectSceneObjectCommand{*infinityObjectId}
                );
            }
            else {
                commandQueue_.enqueue(
                    UpsertPointCloudCommand{
                        "procedural/infinity",
                        InfinityGenerator::generate(infinityConfig_)
                    }
                );
            }

            restorePresentationView(cameraController, false);
            returnToInfinityAfterClear_ = true;
            lessonFeedback_ =
                "Presentation cleared. Infinity is ready.";
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

            ImGui::TextColored(MUTED, "Chionia Lab v%s", CHIONIA_VERSION);
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
