
#pragma once

#include "core/resources/point_cloud.hpp"
#include "shape_sampling_config.hpp"

#include <utility>

namespace chionia {

    class ShapeAsset {
    public:

        ShapeAsset(
            PointCloud pointCloud,
            ShapeSamplingConfig samplingConfig
        )
            : pointCloud_(std::move(pointCloud)),
              samplingConfig_(samplingConfig) {
        }

        const PointCloud& getPointCloud() const {
            return pointCloud_;
        }

        PointCloud& getPointCloud() {
            return pointCloud_;
        }

        const ShapeSamplingConfig& getSamplingConfig() const {
            return samplingConfig_;
        }

        void setSamplingConfig(
            const ShapeSamplingConfig& config
        ) {
            samplingConfig_ = config;
        }

    private:

        PointCloud pointCloud_;
        ShapeSamplingConfig samplingConfig_;
    };

}
