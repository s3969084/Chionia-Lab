
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace chionia {

    using PointId = std::uint64_t;

    struct PointCloudPoint {
        PointId id = 0;
        glm::vec3 position{0.0f};
        glm::vec4 color{1.0f};
        float size = 1.0f;
    };

    struct PointCloudBounds {
        glm::vec3 minimum{0.0f};
        glm::vec3 maximum{0.0f};
    };

    class PointCloud {
    public:
        PointCloud() = default;
        explicit PointCloud(std::string name);

        PointId addPoint(
            const glm::vec3& position,
            const glm::vec4& color = glm::vec4(1.0f),
            float size = 1.0f,
            std::string sourceId = {}
        );

        void clear();

        const std::string& getName() const;
        const std::vector<PointCloudPoint>& getPoints() const;

        std::size_t getPointCount() const;
        bool empty() const;

        // Identifies the current state of the point data.
        // Changes whenever points are added or the cloud is cleared.
        std::uint64_t getRevision() const;

        std::optional<PointId> findBySourceId(
            std::string_view sourceId
        ) const;

        const std::optional<PointCloudBounds>& getBounds() const;

    private:
        static void validatePoint(
            const glm::vec3& position,
            const glm::vec4& color,
            float size
        );

        void expandBounds(const glm::vec3& position);

        std::string name_;
        std::vector<PointCloudPoint> points_;

        // Preserves IDs from imported assets, such as JSON models.
        // These are separate from internal engine point identities.
        std::unordered_map<std::string, PointId> sourceIdMap_;

        PointId nextPointId_ = 1;

        // Incremented whenever the point data changes.
        std::uint64_t revision_ = 0;

        std::optional<PointCloudBounds> bounds_;
    };

}
