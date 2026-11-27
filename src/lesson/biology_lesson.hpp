#pragma once

#include "core/resources/point_cloud.hpp"
#include "lesson/lesson_label.hpp"

#include <array>
#include <string_view>

namespace chionia {

    // A focused, deterministic introduction to the human cell.
    class BiologyLesson {
    public:
        bool recognizesTranscript(std::string_view transcript) const;
        bool applyTranscript(std::string_view transcript);

        void clear();
        bool hasHumanCell() const;

        PointCloud buildPointCloud() const;
        std::array<LessonLabel, 8> getLabels() const;

    private:
        bool humanCellVisible_ = false;
    };

}
