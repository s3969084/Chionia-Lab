#pragma once

#include "core/resources/point_cloud.hpp"
#include "lesson/lesson_label.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace chionia {

    enum class TestLessonTopic {
        None,
        ElectricCircuit,
        CarbonAtom,
        WaterCycle,
        PlateBoundaries,
        MarineFoodWeb,
        PythagoreanTheorem,
        ColdFront
    };

    struct LessonAnimationMarker {
        glm::vec3 position{0.0f};
        glm::vec4 color{1.0f};
    };

    // Temporary procedural scenes used to exercise lesson parsing, labels,
    // scene switching, and subject-specific diagrams.
    class TestLesson {
    public:
        bool recognizesTranscript(std::string_view transcript) const;
        bool applyTranscript(std::string_view transcript);
        void clear();

        bool hasLesson() const;
        TestLessonTopic getTopic() const;
        const char* getTitle() const;
        const char* getSubtitle() const;
        const char* getDescription() const;
        const char* getFeedback() const;

        PointCloud buildPointCloud() const;
        std::vector<LessonLabel> getLabels() const;
        std::optional<LessonAnimationMarker> getAnimationMarker(
            float animationTime
        ) const;

    private:
        TestLessonTopic topic_ = TestLessonTopic::None;
    };

}
