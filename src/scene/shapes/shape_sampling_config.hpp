
#pragma once

#include <cstddef>

namespace chionia {

    enum class ShapeSamplingMode {
        Unknown,
        Curve,
        Surface,
        Volume
    };

    struct ShapeSamplingConfig {

        ShapeSamplingMode mode = ShapeSamplingMode::Unknown;

        // Curve configuration.
        bool closedCurve = false;
        std::size_t curveCount = 1;

        // True when multiple curves are interleaved:
        // A0, B0, A1, B1, A2, B2...
        bool interleavedCurves = false;

        // Optional source prefix to use.
        // Zero means use all source points.
        std::size_t sourcePointCount = 0;

        // Parameterised geometry dimensions.
        // For a volume grid:
        // uCount = X dimension
        // vCount = Y dimension
        // wCount = Z dimension
        std::size_t uCount = 0;
        std::size_t vCount = 0;
        std::size_t wCount = 0;

        bool wrapU = false;
        bool wrapV = false;
    };

}
