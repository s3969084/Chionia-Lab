#pragma once

#include "core/resources/point_cloud.hpp"
#include "lesson/lesson_label.hpp"

#include <array>
#include <string_view>

namespace chionia {

    using SolarSystemLessonLabel = LessonLabel;

    // Small, deterministic lesson-scene prototype. Live transcription can
    // feed finalized speech segments into applyTranscript() later.
    class SolarSystemLesson {
    public:
        bool recognizesTranscript(std::string_view transcript) const;
        bool applyTranscript(std::string_view transcript);

        void clear();

        bool hasSolarSystem() const;
        bool hasGravityExplanation() const;

        PointCloud buildPointCloud() const;
        std::array<LessonLabel, 11> getLabels(
            float elapsedTime
        ) const;

    private:
        bool solarSystemVisible_ = false;
        bool gravityExplanationVisible_ = false;
    };

}
