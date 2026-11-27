#pragma once

#include "core/resources/point_cloud.hpp"

#include <array>
#include <string_view>

namespace chionia {

    struct SolarSystemLessonLabel {
        std::string_view text;
        glm::vec3 position{0.0f};
        glm::vec4 color{1.0f};
    };

    // Small, deterministic lesson-scene prototype. Live transcription can
    // feed finalized speech segments into applyTranscript() later.
    class SolarSystemLesson {
    public:
        bool applyTranscript(std::string_view transcript);

        void clear();

        bool hasSolarSystem() const;
        bool hasGravityExplanation() const;

        PointCloud buildPointCloud() const;
        std::array<SolarSystemLessonLabel, 11> getLabels(
            float elapsedTime
        ) const;

    private:
        bool solarSystemVisible_ = false;
        bool gravityExplanationVisible_ = false;
    };

}
