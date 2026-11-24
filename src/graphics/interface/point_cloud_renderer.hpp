
#pragma once

#include "core/resources/point_cloud.hpp"

#include <cstddef>

namespace chionia {

    class PointCloudRenderer {
    public:
        virtual ~PointCloudRenderer() = default;

        // Synchronize the GPU representation with the supplied
        // CPU-side PointCloud.
        //
        // The implementation decides whether to:
        // - Allocate GPU resources.
        // - Reuse existing capacity.
        // - Grow GPU buffers.
        // - Upload changed point data.
        virtual void synchronize(
            const PointCloud& pointCloud
        ) = 0;

        // Release renderer-owned GPU resources.
        virtual void release() = 0;

        // Number of points currently resident in the GPU representation.
        virtual std::size_t getResidentPointCount() const = 0;
    };

}
