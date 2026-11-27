#pragma once

#include "core/resources/point_cloud.hpp"
#include "lesson/lesson_label.hpp"

#include <array>
#include <string_view>

namespace chionia {

    // A schematic lecture model of the four heart chambers and circulation.
    class AnatomyLesson {
    public:
        bool recognizesTranscript(std::string_view transcript) const;
        bool applyTranscript(std::string_view transcript);

        void clear();
        bool hasHeartCycle() const;

        PointCloud buildPointCloud() const;
        std::array<LessonLabel, 14> getLabels() const;

    private:
        bool heartCycleVisible_ = false;
    };

}
