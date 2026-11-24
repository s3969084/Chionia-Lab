
#include "point_cloud.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace chionia {

    PointCloud::PointCloud(std::string name)
        : name_(std::move(name)) {
    }

    PointId PointCloud::addPoint(
        const glm::vec3& position,
        const glm::vec4& color,
        float size,
        std::string sourceId
    ) {
        validatePoint(position, color, size);

        if (
            !sourceId.empty() &&
            sourceIdMap_.find(sourceId) != sourceIdMap_.end()
        ) {
            throw std::invalid_argument(
                "PointCloud contains a duplicate source ID: " + sourceId
            );
        }

        if (nextPointId_ == 0) {
            throw std::overflow_error(
                "PointCloud point ID space exhausted."
            );
        }

        const PointId id = nextPointId_++;

        PointCloudPoint point;
        point.id = id;
        point.position = position;
        point.color = color;
        point.size = size;

        points_.push_back(point);

        if (!sourceId.empty()) {
            sourceIdMap_.emplace(std::move(sourceId), id);
        }

        expandBounds(position);

        ++revision_;

        return id;
    }

    void PointCloud::clear() {
        if (points_.empty()) {
            return;
        }

        points_.clear();
        sourceIdMap_.clear();
        bounds_.reset();

        // Keep nextPointId_ increasing so IDs are not reused
        // during this PointCloud object's lifetime.
        ++revision_;
    }

    const std::string& PointCloud::getName() const {
        return name_;
    }

    const std::vector<PointCloudPoint>& PointCloud::getPoints() const {
        return points_;
    }

    std::size_t PointCloud::getPointCount() const {
        return points_.size();
    }

    bool PointCloud::empty() const {
        return points_.empty();
    }

    std::uint64_t PointCloud::getRevision() const {
        return revision_;
    }

    std::optional<PointId> PointCloud::findBySourceId(
        std::string_view sourceId
    ) const {
        const auto it = sourceIdMap_.find(std::string(sourceId));

        if (it == sourceIdMap_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    const std::optional<PointCloudBounds>&
    PointCloud::getBounds() const {
        return bounds_;
    }

    void PointCloud::validatePoint(
        const glm::vec3& position,
        const glm::vec4& color,
        float size
    ) {
        const bool validPosition =
            std::isfinite(position.x) &&
            std::isfinite(position.y) &&
            std::isfinite(position.z);

        const bool validColor =
            std::isfinite(color.r) &&
            std::isfinite(color.g) &&
            std::isfinite(color.b) &&
            std::isfinite(color.a);

        if (!validPosition) {
            throw std::invalid_argument(
                "PointCloud point has an invalid position."
            );
        }

        if (!validColor) {
            throw std::invalid_argument(
                "PointCloud point has an invalid colour."
            );
        }

        if (!std::isfinite(size) || size <= 0.0f) {
            throw std::invalid_argument(
                "PointCloud point size must be positive and finite."
            );
        }
    }

    void PointCloud::expandBounds(const glm::vec3& position) {
        if (!bounds_) {
            bounds_ = PointCloudBounds{
                position,
                position
            };
            return;
        }

        bounds_->minimum = glm::min(bounds_->minimum, position);
        bounds_->maximum = glm::max(bounds_->maximum, position);
    }

}
