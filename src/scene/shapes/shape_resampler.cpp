
#include "shape_resampler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace chionia {

    namespace {

        constexpr double DUPLICATE_EPSILON = 1e-12;
        constexpr double MINIMUM_TRIANGLE_AREA = 1e-15;

        bool isValidPosition(const glm::vec3& position) {
            return
                std::isfinite(position.x) &&
                std::isfinite(position.y) &&
                std::isfinite(position.z);
        }

        double distanceBetween(
            const glm::vec3& a,
            const glm::vec3& b
        ) {
            const double dx = static_cast<double>(b.x) - a.x;
            const double dy = static_cast<double>(b.y) - a.y;
            const double dz = static_cast<double>(b.z) - a.z;

            return std::hypot(dx, dy, dz);
        }

        std::vector<glm::vec3> extractPositions(
            const PointCloud& pointCloud
        ) {
            std::vector<glm::vec3> positions;
            positions.reserve(pointCloud.getPointCount());

            for (const PointCloudPoint& point : pointCloud.getPoints()) {
                positions.push_back(point.position);
            }

            return positions;
        }

        std::vector<glm::vec3> filterValidPositions(
            const std::vector<glm::vec3>& source
        ) {
            std::vector<glm::vec3> result;
            result.reserve(source.size());

            for (const glm::vec3& position : source) {
                if (isValidPosition(position)) {
                    result.push_back(position);
                }
            }

            return result;
        }

        // ------------------------------------------------------------
        // CURVE RESAMPLING
        // ------------------------------------------------------------

        double calculateCurveLength(
            const std::vector<glm::vec3>& points,
            bool closed
        ) {
            if (points.size() < 2) {
                return 0.0;
            }

            double length = 0.0;

            for (std::size_t i = 1; i < points.size(); ++i) {
                length += distanceBetween(points[i - 1], points[i]);
            }

            if (closed) {
                length += distanceBetween(points.back(), points.front());
            }

            return length;
        }

        std::vector<glm::vec3> resampleCurve(
            std::vector<glm::vec3> points,
            std::size_t targetCount,
            bool closed
        ) {
            if (targetCount == 0 || points.empty()) {
                throw std::invalid_argument(
                    "Curve resampling requires points and a positive target count."
                );
            }

            if (closed && points.size() > 1) {
                const double closingDistance =
                    distanceBetween(points.front(), points.back());

                if (closingDistance <= DUPLICATE_EPSILON) {
                    points.pop_back();
                }
            }

            if (closed && points.size() < 3) {
                throw std::invalid_argument(
                    "A closed curve requires at least three distinct points."
                );
            }

            if (!closed && points.size() < 2) {
                throw std::invalid_argument(
                    "An open curve requires at least two points."
                );
            }

            if (targetCount == 1) {
                return { points.front() };
            }

            const std::size_t pointCount = points.size();

            const std::size_t segmentCount =
                closed ? pointCount : pointCount - 1;

            std::vector<double> cumulativeDistances(
                segmentCount + 1,
                0.0
            );

            for (std::size_t i = 0; i < segmentCount; ++i) {
                const glm::vec3& a = points[i];
                const glm::vec3& b = points[(i + 1) % pointCount];

                cumulativeDistances[i + 1] =
                    cumulativeDistances[i] + distanceBetween(a, b);
            }

            const double totalLength = cumulativeDistances.back();

            if (totalLength <= 0.0) {
                throw std::invalid_argument(
                    "Cannot resample a curve with zero length."
                );
            }

            std::vector<glm::vec3> result;
            result.reserve(targetCount);

            const double denominator =
                closed
                ? static_cast<double>(targetCount)
                : static_cast<double>(targetCount - 1);

            for (std::size_t i = 0; i < targetCount; ++i) {
                const double targetDistance =
                    totalLength * static_cast<double>(i) / denominator;

                const auto upper = std::upper_bound(
                    cumulativeDistances.begin(),
                    cumulativeDistances.end(),
                    targetDistance
                );

                std::size_t segmentIndex =
                    static_cast<std::size_t>(
                        upper - cumulativeDistances.begin()
                    );

                if (segmentIndex == 0) {
                    segmentIndex = 1;
                }

                if (segmentIndex > segmentCount) {
                    segmentIndex = segmentCount;
                }

                const std::size_t indexA = segmentIndex - 1;
                const std::size_t indexB = segmentIndex % pointCount;

                const double segmentStart = cumulativeDistances[indexA];
                const double segmentEnd = cumulativeDistances[segmentIndex];
                const double segmentLength = segmentEnd - segmentStart;

                float t = 0.0f;

                if (segmentLength > 0.0) {
                    t = static_cast<float>(
                        (targetDistance - segmentStart) / segmentLength
                    );
                }

                result.push_back(
                    glm::mix(points[indexA], points[indexB], t)
                );
            }

            return result;
        }

        std::vector<std::size_t> allocateParticleCounts(
            const std::vector<double>& curveLengths,
            std::size_t totalCount,
            std::size_t minimumPerCurve
        ) {
            const std::size_t curveCount = curveLengths.size();

            if (
                curveCount == 0 ||
                curveCount * minimumPerCurve > totalCount
            ) {
                throw std::invalid_argument(
                    "Invalid curve count for particle allocation."
                );
            }

            const double totalLength = std::accumulate(
                curveLengths.begin(),
                curveLengths.end(),
                0.0
            );

            if (totalLength <= 0.0) {
                throw std::invalid_argument(
                    "Cannot allocate particles to zero-length curves."
                );
            }

            std::vector<std::size_t> allocations(
                curveCount,
                minimumPerCurve
            );

            const std::size_t remaining =
                totalCount - curveCount * minimumPerCurve;

            struct Remainder {
                std::size_t index;
                double fraction;
            };

            std::vector<Remainder> remainders;
            remainders.reserve(curveCount);

            std::size_t allocated = 0;

            for (std::size_t i = 0; i < curveCount; ++i) {
                const double exactShare =
                    static_cast<double>(remaining) *
                    curveLengths[i] / totalLength;

                const std::size_t wholeShare =
                    static_cast<std::size_t>(std::floor(exactShare));

                allocations[i] += wholeShare;
                allocated += wholeShare;

                remainders.push_back({
                    i,
                    exactShare - static_cast<double>(wholeShare)
                });
            }

            std::sort(
                remainders.begin(),
                remainders.end(),
                [](const Remainder& a, const Remainder& b) {
                    return a.fraction > b.fraction;
                }
            );

            const std::size_t leftover = remaining - allocated;

            for (std::size_t i = 0; i < leftover; ++i) {
                ++allocations[remainders[i].index];
            }

            return allocations;
        }

        std::vector<std::vector<glm::vec3>> splitCurves(
            const std::vector<glm::vec3>& source,
            std::size_t curveCount,
            bool interleaved
        ) {
            if (curveCount == 0 || source.size() < curveCount) {
                throw std::invalid_argument(
                    "Invalid source data for multiple curves."
                );
            }

            std::vector<std::vector<glm::vec3>> curves(curveCount);

            if (interleaved) {
                for (std::size_t i = 0; i < source.size(); ++i) {
                    curves[i % curveCount].push_back(source[i]);
                }
            }
            else {
                const std::size_t baseCount = source.size() / curveCount;
                const std::size_t remainder = source.size() % curveCount;

                std::size_t offset = 0;

                for (std::size_t i = 0; i < curveCount; ++i) {
                    const std::size_t count =
                        baseCount + (i < remainder ? 1 : 0);

                    curves[i].insert(
                        curves[i].end(),
                        source.begin() + offset,
                        source.begin() + offset + count
                    );

                    offset += count;
                }
            }

            return curves;
        }

        std::vector<glm::vec3> resampleConfiguredCurves(
            const std::vector<glm::vec3>& source,
            std::size_t curveCount,
            bool interleaved,
            bool closed,
            std::size_t sourcePointCount = 0
        ) {
            if (
                sourcePointCount > 0 &&
                sourcePointCount > source.size()
            ) {
                throw std::invalid_argument(
                    "Configured source point count exceeds available points."
                );
            }

            std::vector<glm::vec3> selectedSource(
                source.begin(),
                sourcePointCount == 0
                    ? source.end()
                    : source.begin() + sourcePointCount
            );

            std::vector<glm::vec3> validSource =
                filterValidPositions(selectedSource);

            if (validSource.empty()) {
                throw std::invalid_argument(
                    "Shape contains no valid coordinates."
                );
            }

            auto curves = splitCurves(
                validSource,
                curveCount,
                interleaved
            );

            std::vector<double> lengths;
            lengths.reserve(curves.size());

            for (const auto& curve : curves) {
                const double length = calculateCurveLength(curve, closed);

                if (length <= 0.0) {
                    throw std::invalid_argument(
                        "A source curve has zero length."
                    );
                }

                lengths.push_back(length);
            }

            const std::size_t minimumPerCurve = closed ? 3 : 2;

            const auto allocations = allocateParticleCounts(
                lengths,
                ShapeResampler::PARTICLE_COUNT,
                minimumPerCurve
            );

            std::vector<glm::vec3> result;
            result.reserve(ShapeResampler::PARTICLE_COUNT);

            for (std::size_t i = 0; i < curves.size(); ++i) {
                auto sampled = resampleCurve(
                    std::move(curves[i]),
                    allocations[i],
                    closed
                );

                result.insert(
                    result.end(),
                    sampled.begin(),
                    sampled.end()
                );
            }

            if (result.size() != ShapeResampler::PARTICLE_COUNT) {
                throw std::runtime_error(
                    "Curve resampler produced an incorrect particle count."
                );
            }

            return result;
        }

        // ------------------------------------------------------------
        // LOW-DISCREPANCY SEQUENCE
        // ------------------------------------------------------------

        double radicalInverse(
            std::size_t index,
            std::size_t base
        ) {
            double result = 0.0;
            double fraction = 1.0 / static_cast<double>(base);

            while (index > 0) {
                result +=
                    static_cast<double>(index % base) * fraction;

                index /= base;
                fraction /= static_cast<double>(base);
            }

            return result;
        }

        // ------------------------------------------------------------
        // VOLUME GRID RESAMPLING
        // ------------------------------------------------------------

        std::vector<glm::vec3> resampleVolumeGrid(
            const std::vector<glm::vec3>& source,
            std::size_t uCount,
            std::size_t vCount,
            std::size_t wCount
        ) {
            if (uCount < 2 || vCount < 2 || wCount < 2) {
                throw std::invalid_argument(
                    "Volume grid dimensions must each be at least two."
                );
            }

            if (
                uCount > std::numeric_limits<std::size_t>::max() / vCount
            ) {
                throw std::invalid_argument(
                    "Volume grid dimensions overflow."
                );
            }

            const std::size_t uvCount = uCount * vCount;

            if (
                uvCount > std::numeric_limits<std::size_t>::max() / wCount
            ) {
                throw std::invalid_argument(
                    "Volume grid dimensions overflow."
                );
            }

            const std::size_t expectedCount = uvCount * wCount;

            if (source.size() != expectedCount) {
                throw std::invalid_argument(
                    "Volume grid dimensions do not match source point count."
                );
            }

            for (const glm::vec3& position : source) {
                if (!isValidPosition(position)) {
                    throw std::invalid_argument(
                        "Volume grid contains invalid coordinates."
                    );
                }
            }

            // X outermost, Y middle, Z innermost.
            const auto gridPoint = [&](
                std::size_t x,
                std::size_t y,
                std::size_t z
            ) -> const glm::vec3& {
                const std::size_t index =
                    (x * vCount + y) * wCount + z;

                return source[index];
            };

            std::vector<glm::vec3> result;
            result.reserve(ShapeResampler::PARTICLE_COUNT);

            for (
                std::size_t i = 0;
                i < ShapeResampler::PARTICLE_COUNT;
                ++i
            ) {
                const double sampleU = radicalInverse(i + 1, 2);
                const double sampleV = radicalInverse(i + 1, 3);
                const double sampleW = radicalInverse(i + 1, 5);

                const double gridU =
                    sampleU * static_cast<double>(uCount - 1);

                const double gridV =
                    sampleV * static_cast<double>(vCount - 1);

                const double gridW =
                    sampleW * static_cast<double>(wCount - 1);

                const std::size_t x0 = std::min(
                    static_cast<std::size_t>(gridU),
                    uCount - 2
                );

                const std::size_t y0 = std::min(
                    static_cast<std::size_t>(gridV),
                    vCount - 2
                );

                const std::size_t z0 = std::min(
                    static_cast<std::size_t>(gridW),
                    wCount - 2
                );

                const std::size_t x1 = x0 + 1;
                const std::size_t y1 = y0 + 1;
                const std::size_t z1 = z0 + 1;

                const float tx = static_cast<float>(gridU - x0);
                const float ty = static_cast<float>(gridV - y0);
                const float tz = static_cast<float>(gridW - z0);

                const glm::vec3 z00 = glm::mix(
                    gridPoint(x0, y0, z0),
                    gridPoint(x0, y0, z1),
                    tz
                );

                const glm::vec3 z10 = glm::mix(
                    gridPoint(x1, y0, z0),
                    gridPoint(x1, y0, z1),
                    tz
                );

                const glm::vec3 z01 = glm::mix(
                    gridPoint(x0, y1, z0),
                    gridPoint(x0, y1, z1),
                    tz
                );

                const glm::vec3 z11 = glm::mix(
                    gridPoint(x1, y1, z0),
                    gridPoint(x1, y1, z1),
                    tz
                );

                const glm::vec3 y0Result = glm::mix(z00, z01, ty);
                const glm::vec3 y1Result = glm::mix(z10, z11, ty);

                result.push_back(glm::mix(y0Result, y1Result, tx));
            }

            return result;
        }

        // ------------------------------------------------------------
        // SURFACE GRID RESAMPLING
        // ------------------------------------------------------------

        struct SurfaceTriangle {
            glm::vec3 a;
            glm::vec3 b;
            glm::vec3 c;
            double cumulativeArea;
        };

        double calculateTriangleArea(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c
        ) {
            const double abX = static_cast<double>(b.x) - a.x;
            const double abY = static_cast<double>(b.y) - a.y;
            const double abZ = static_cast<double>(b.z) - a.z;

            const double acX = static_cast<double>(c.x) - a.x;
            const double acY = static_cast<double>(c.y) - a.y;
            const double acZ = static_cast<double>(c.z) - a.z;

            const double crossX = abY * acZ - abZ * acY;
            const double crossY = abZ * acX - abX * acZ;
            const double crossZ = abX * acY - abY * acX;

            return 0.5 * std::sqrt(
                crossX * crossX +
                crossY * crossY +
                crossZ * crossZ
            );
        }

        std::vector<glm::vec3> resampleSurfaceGrid(
            const std::vector<glm::vec3>& source,
            std::size_t uCount,
            std::size_t vCount,
            bool wrapU,
            bool wrapV
        ) {
            if (uCount < 2 || vCount < 2) {
                throw std::invalid_argument(
                    "Surface grid dimensions must each be at least two."
                );
            }

            if (
                uCount > std::numeric_limits<std::size_t>::max() / vCount
            ) {
                throw std::invalid_argument(
                    "Surface grid dimensions overflow."
                );
            }

            if (source.size() != uCount * vCount) {
                throw std::invalid_argument(
                    "Surface grid dimensions do not match source point count."
                );
            }

            for (const glm::vec3& position : source) {
                if (!isValidPosition(position)) {
                    throw std::invalid_argument(
                        "Surface grid contains invalid coordinates."
                    );
                }
            }

            // A wrapped dimension is expected to contain a duplicated
            // final row or column representing its starting seam.
            const std::size_t effectiveU = wrapU ? uCount - 1 : uCount;
            const std::size_t effectiveV = wrapV ? vCount - 1 : vCount;

            if (effectiveU < 2 || effectiveV < 2) {
                throw std::invalid_argument(
                    "Surface grid has insufficient unique samples."
                );
            }

            const auto gridPoint = [&](
                std::size_t u,
                std::size_t v
            ) -> const glm::vec3& {
                return source[v * uCount + u];
            };

            const std::size_t uCellCount =
                wrapU ? effectiveU : effectiveU - 1;

            const std::size_t vCellCount =
                wrapV ? effectiveV : effectiveV - 1;

            std::vector<SurfaceTriangle> triangles;
            triangles.reserve(uCellCount * vCellCount * 2);

            double totalArea = 0.0;

            const auto addTriangle = [&](
                const glm::vec3& a,
                const glm::vec3& b,
                const glm::vec3& c
            ) {
                const double area = calculateTriangleArea(a, b, c);

                // Degenerate triangles occur naturally at sphere poles.
                if (area <= MINIMUM_TRIANGLE_AREA) {
                    return;
                }

                totalArea += area;

                triangles.push_back({
                    a,
                    b,
                    c,
                    totalArea
                });
            };

            for (std::size_t v = 0; v < vCellCount; ++v) {
                const std::size_t nextV = (v + 1) % effectiveV;

                for (std::size_t u = 0; u < uCellCount; ++u) {
                    const std::size_t nextU = (u + 1) % effectiveU;

                    const glm::vec3& a = gridPoint(u, v);
                    const glm::vec3& b = gridPoint(nextU, v);
                    const glm::vec3& c = gridPoint(nextU, nextV);
                    const glm::vec3& d = gridPoint(u, nextV);

                    addTriangle(a, b, c);
                    addTriangle(a, c, d);
                }
            }

            if (triangles.empty() || totalArea <= 0.0) {
                throw std::invalid_argument(
                    "Surface grid contains no usable triangle area."
                );
            }

            // Select triangles proportionally to their actual area,
            // then sample uniformly inside each selected triangle.
            std::vector<glm::vec3> result;
            result.reserve(ShapeResampler::PARTICLE_COUNT);

            for (
                std::size_t i = 0;
                i < ShapeResampler::PARTICLE_COUNT;
                ++i
            ) {
                const double areaSample =
                    radicalInverse(i + 1, 2) * totalArea;

                const auto triangleIt = std::lower_bound(
                    triangles.begin(),
                    triangles.end(),
                    areaSample,
                    [](const SurfaceTriangle& triangle, double value) {
                        return triangle.cumulativeArea < value;
                    }
                );

                const SurfaceTriangle& triangle =
                    triangleIt == triangles.end()
                    ? triangles.back()
                    : *triangleIt;

                const double r1 = radicalInverse(i + 1, 3);
                const double r2 = radicalInverse(i + 1, 5);

                const double sqrtR1 = std::sqrt(r1);

                const float weightA =
                    static_cast<float>(1.0 - sqrtR1);

                const float weightB =
                    static_cast<float>(sqrtR1 * (1.0 - r2));

                const float weightC =
                    static_cast<float>(sqrtR1 * r2);

                result.push_back(
                    triangle.a * weightA +
                    triangle.b * weightB +
                    triangle.c * weightC
                );
            }

            return result;
        }

        // ------------------------------------------------------------
        // LEGACY RESAMPLING
        // ------------------------------------------------------------

        std::vector<glm::vec3> resamplePositions(
            const std::vector<glm::vec3>& source
        ) {
            return resampleConfiguredCurves(
                source,
                1,
                false,
                false
            );
        }

    }

    // ------------------------------------------------------------
    // PUBLIC RESAMPLING INTERFACE
    // ------------------------------------------------------------

    std::vector<glm::vec3> ShapeResampler::resample(
        const ShapeAsset& asset
    ) {
        const ShapeSamplingConfig& config =
            asset.getSamplingConfig();

        const std::vector<glm::vec3> positions =
            extractPositions(asset.getPointCloud());

        switch (config.mode) {

            case ShapeSamplingMode::Curve:
                return resampleConfiguredCurves(
                    positions,
                    config.curveCount,
                    config.interleavedCurves,
                    config.closedCurve,
                    config.sourcePointCount
                );

            case ShapeSamplingMode::Volume:
                return resampleVolumeGrid(
                    positions,
                    config.uCount,
                    config.vCount,
                    config.wCount
                );

            case ShapeSamplingMode::Surface:
                return resampleSurfaceGrid(
                    positions,
                    config.uCount,
                    config.vCount,
                    config.wrapU,
                    config.wrapV
                );

            case ShapeSamplingMode::Unknown:
            default:
                throw std::invalid_argument(
                    "Cannot resample ShapeAsset: sampling mode is Unknown."
                );
        }
    }

    std::vector<glm::vec3> ShapeResampler::resample(
        const PointCloud& pointCloud
    ) {
        return resamplePositions(extractPositions(pointCloud));
    }

    std::vector<glm::vec3> ShapeResampler::resample(
        const std::vector<Point>& points
    ) {
        std::vector<glm::vec3> positions;
        positions.reserve(points.size());

        for (const Point& point : points) {
            positions.push_back(point.position);
        }

        return resamplePositions(positions);
    }

}
