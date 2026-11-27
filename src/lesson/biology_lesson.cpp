#include "biology_lesson.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace chionia {

    namespace {

        constexpr float PI = 3.14159265359f;
        constexpr float GOLDEN_ANGLE = 2.39996322973f;
        constexpr glm::vec3 CELL_RADII(0.78f, 0.62f, 0.54f);
        constexpr glm::vec3 NUCLEUS_CENTER(-0.08f, 0.035f, 0.025f);
        constexpr glm::vec3 NUCLEUS_RADII(0.31f, 0.27f, 0.24f);

        std::string lowercase(std::string_view text) {
            std::string result;
            result.reserve(text.size());
            for (const unsigned char character : text) {
                if (character >= 'A' && character <= 'Z') {
                    result.push_back(static_cast<char>(character - 'A' + 'a'));
                }
                else {
                    result.push_back(static_cast<char>(character));
                }
            }
            return result;
        }

        glm::vec3 sphereDirection(std::size_t index, std::size_t count) {
            const float fraction =
                (static_cast<float>(index) + 0.5f) /
                static_cast<float>(count);
            const float y = 1.0f - 2.0f * fraction;
            const float ringRadius = std::sqrt(
                std::max(0.0f, 1.0f - y * y)
            );
            const float angle = GOLDEN_ANGLE * static_cast<float>(index);
            return glm::vec3(
                std::cos(angle) * ringRadius,
                y,
                std::sin(angle) * ringRadius
            );
        }

        glm::vec3 ellipsoidPoint(
            const glm::vec3& direction,
            const glm::vec3& radii,
            const glm::vec3& center = glm::vec3(0.0f)
        ) {
            return center + direction * radii;
        }

        void addPlasmaMembrane(PointCloud& cloud) {
            // One complete, thin plasma-membrane boundary. The cell interior
            // stays visible through the point-cloud renderer's additive pass.
            constexpr std::size_t POINTS = 19000;
            for (std::size_t index = 0; index < POINTS; ++index) {
                const glm::vec3 direction = sphereDirection(index, POINTS);
                const float variation = 0.5f + 0.5f * std::sin(
                    static_cast<float>(index) * 0.043f
                );
                const glm::vec3 color = glm::mix(
                    glm::vec3(0.012f, 0.22f, 0.31f),
                    glm::vec3(0.025f, 0.36f, 0.46f),
                    variation
                );
                cloud.addPoint(
                    ellipsoidPoint(direction, CELL_RADII),
                    glm::vec4(color, 0.72f)
                );
            }
        }

        void addCytoplasm(PointCloud& cloud, std::size_t pointCount) {
            // Show a faint sample of the cytosol instead of a second bright
            // spherical shell. The remaining points keep subject morphs at a
            // stable particle count but are transparent black anchors.
            const std::size_t visibleCount = std::min<std::size_t>(
                pointCount,
                18000
            );
            std::size_t added = 0;
            for (std::size_t index = 0; added < visibleCount; ++index) {
                const float volumeFraction = std::fmod(
                    static_cast<float>(index) * 0.61803398875f + 0.5f,
                    1.0f
                );
                const float radius = 0.93f * std::cbrt(volumeFraction);
                const glm::vec3 direction = sphereDirection(
                    index,
                    visibleCount * 2
                );
                const glm::vec3 position = ellipsoidPoint(
                    direction * radius,
                    CELL_RADII
                );
                const glm::vec3 nucleusRelative =
                    (position - NUCLEUS_CENTER) / NUCLEUS_RADII;
                if (glm::dot(nucleusRelative, nucleusRelative) < 1.12f) {
                    continue;
                }

                const float variation = 0.5f + 0.5f * std::sin(
                    static_cast<float>(index) * 0.17f
                );
                cloud.addPoint(
                    position,
                    glm::vec4(
                        0.0008f,
                        0.003f + variation * 0.001f,
                        0.005f + variation * 0.002f,
                        0.30f
                    )
                );
                ++added;
            }

            for (std::size_t index = visibleCount; index < pointCount; ++index) {
                const glm::vec3 direction = sphereDirection(
                    index - visibleCount,
                    std::max<std::size_t>(pointCount - visibleCount, 1)
                );
                cloud.addPoint(
                    ellipsoidPoint(direction * 0.42f, CELL_RADII),
                    glm::vec4(0.0f)
                );
            }
        }

        void addNucleus(PointCloud& cloud) {
            // A single visible nuclear boundary represents the nuclear
            // envelope without drawing two bright concentric membranes.
            constexpr std::size_t ENVELOPE_POINTS = 6400;
            for (std::size_t index = 0; index < ENVELOPE_POINTS; ++index) {
                const glm::vec3 direction = sphereDirection(
                    index,
                    ENVELOPE_POINTS
                );
                const float variation = 0.5f + 0.5f * std::sin(
                    static_cast<float>(index) * 0.091f
                );
                const glm::vec3 color = glm::mix(
                    glm::vec3(0.21f, 0.055f, 0.34f),
                    glm::vec3(0.42f, 0.13f, 0.54f),
                    variation
                );
                cloud.addPoint(
                    ellipsoidPoint(
                        direction,
                        NUCLEUS_RADII,
                        NUCLEUS_CENTER
                    ),
                    glm::vec4(color, 0.52f)
                );
            }

            constexpr std::size_t NUCLEOLUS_POINTS = 1050;
            const glm::vec3 nucleolusCenter = NUCLEUS_CENTER +
                glm::vec3(-0.075f, 0.025f, 0.045f);
            constexpr glm::vec3 NUCLEOLUS_RADII(0.062f, 0.052f, 0.050f);
            for (std::size_t index = 0; index < NUCLEOLUS_POINTS; ++index) {
                const glm::vec3 direction = sphereDirection(
                    index,
                    NUCLEOLUS_POINTS
                );
                cloud.addPoint(
                    ellipsoidPoint(
                        direction,
                        NUCLEOLUS_RADII,
                        nucleolusCenter
                    ),
                    glm::vec4(0.42f, 0.10f, 0.27f, 0.72f)
                );
            }
        }

        void addChromatin(PointCloud& cloud) {
            // Interphase DNA is represented as compact, irregular chromatin
            // threads inside the nucleus, rather than a giant naked helix.
            constexpr int THREADS = 4;
            constexpr int STEPS = 360;
            for (int thread = 0; thread < THREADS; ++thread) {
                const float phase = static_cast<float>(thread) * 1.71f;
                for (int step = 0; step < STEPS; ++step) {
                    const float fraction = static_cast<float>(step) /
                        static_cast<float>(STEPS - 1);
                    const float angle = fraction * 6.0f * PI + phase;
                    const float radiusX = 0.055f +
                        0.018f * std::sin(angle * 1.7f + phase);
                    const float radiusZ = 0.050f +
                        0.015f * std::cos(angle * 1.3f - phase);
                    const glm::vec3 position(
                        NUCLEUS_CENTER.x + std::cos(angle) * radiusX,
                        NUCLEUS_CENTER.y + (fraction - 0.5f) * 0.16f +
                            0.014f * std::sin(angle * 2.0f),
                        NUCLEUS_CENTER.z + std::sin(angle) * radiusZ
                    );
                    cloud.addPoint(
                        position,
                        thread % 2 == 0
                            ? glm::vec4(0.49f, 0.24f, 0.69f, 0.78f)
                            : glm::vec4(0.33f, 0.18f, 0.58f, 0.74f)
                    );
                }
            }
        }

        glm::vec3 rotateAboutZ(const glm::vec3& point, float angle) {
            const float cosine = std::cos(angle);
            const float sine = std::sin(angle);
            return glm::vec3(
                point.x * cosine - point.y * sine,
                point.x * sine + point.y * cosine,
                point.z
            );
        }

        void addMitochondrion(
            PointCloud& cloud,
            const glm::vec3& center,
            float angle
        ) {
            // Oval outer profile plus a few folded inner-membrane cristae.
            constexpr std::size_t OUTER_POINTS = 1180;
            constexpr glm::vec3 RADII(0.13f, 0.073f, 0.056f);
            for (std::size_t index = 0; index < OUTER_POINTS; ++index) {
                const glm::vec3 direction = sphereDirection(
                    index,
                    OUTER_POINTS
                );
                const glm::vec3 local = ellipsoidPoint(direction, RADII);
                const float variation = 0.5f + 0.5f * std::sin(
                    static_cast<float>(index) * 0.13f
                );
                const glm::vec3 color = glm::mix(
                    glm::vec3(0.34f, 0.095f, 0.018f),
                    glm::vec3(0.58f, 0.24f, 0.045f),
                    variation
                );
                cloud.addPoint(
                    center + rotateAboutZ(local, angle),
                    glm::vec4(color, 0.76f)
                );
            }

            constexpr int CRISTAE = 4;
            constexpr int STEPS = 90;
            for (int fold = 0; fold < CRISTAE; ++fold) {
                const float baseOffset =
                    (static_cast<float>(fold) - 1.5f) * 0.024f;
                for (int step = 0; step < STEPS; ++step) {
                    const float fraction = static_cast<float>(step) /
                        static_cast<float>(STEPS - 1);
                    const float x = (fraction - 0.5f) * 0.19f;
                    const float y = baseOffset +
                        std::sin(fraction * 4.0f * PI + fold * 0.45f) * 0.010f;
                    cloud.addPoint(
                        center + rotateAboutZ(glm::vec3(x, y, 0.012f), angle),
                        glm::vec4(0.73f, 0.42f, 0.10f, 0.82f)
                    );
                }
            }
        }

        void addEndoplasmicReticulum(PointCloud& cloud) {
            // Three open, folded cisternae sit beside the nucleus. They read
            // as ER folds rather than complete orbital rings or extra shells.
            constexpr int FOLDS = 3;
            constexpr int STEPS = 250;
            for (int fold = 0; fold < FOLDS; ++fold) {
                const float radiusX = 0.31f +
                    static_cast<float>(fold) * 0.042f;
                const float radiusY = 0.265f +
                    static_cast<float>(fold) * 0.036f;
                for (int step = 0; step < STEPS; ++step) {
                    const float fraction = static_cast<float>(step) /
                        static_cast<float>(STEPS - 1);
                    const float angle =
                        (-0.61f + 1.22f * fraction) * PI;
                    const float ripple = 0.012f *
                        std::sin(fraction * 6.0f * PI + fold * 0.8f);
                    const glm::vec3 position(
                        NUCLEUS_CENTER.x + std::cos(angle) * radiusX,
                        NUCLEUS_CENTER.y +
                            std::sin(angle) * (radiusY + ripple),
                        0.025f + static_cast<float>(fold) * 0.025f
                    );
                    cloud.addPoint(
                        position,
                        fold == 0
                            ? glm::vec4(0.055f, 0.30f, 0.48f, 0.72f)
                            : glm::vec4(0.09f, 0.22f, 0.39f, 0.68f)
                    );
                }
            }
        }

        void addRibosomes(PointCloud& cloud) {
            // Ribosomes are tiny free dots and dots attached to the rough ER.
            constexpr std::size_t FREE_RIBOSOMES = 900;
            for (std::size_t index = 0; index < FREE_RIBOSOMES; ++index) {
                const glm::vec3 direction = sphereDirection(
                    index,
                    FREE_RIBOSOMES * 2
                );
                const float radialFraction = std::fmod(
                    static_cast<float>(index) * 0.754877666f + 0.17f,
                    1.0f
                );
                const float radius = 0.42f + 0.42f *
                    std::cbrt(radialFraction);
                const glm::vec3 position = ellipsoidPoint(
                    direction * radius,
                    CELL_RADII
                );
                const glm::vec3 nucleusRelative =
                    (position - NUCLEUS_CENTER) / NUCLEUS_RADII;
                if (glm::dot(nucleusRelative, nucleusRelative) < 1.35f) {
                    continue;
                }
                cloud.addPoint(
                    position,
                    glm::vec4(0.62f, 0.20f, 0.29f, 0.74f)
                );
            }

            constexpr int ER_BOUND_RIBOSOMES = 220;
            for (int index = 0; index < ER_BOUND_RIBOSOMES; ++index) {
                const float fraction = static_cast<float>(index) /
                    static_cast<float>(ER_BOUND_RIBOSOMES - 1);
                const float angle = (-0.61f + 1.22f * fraction) * PI;
                const float radiusX = 0.31f;
                const float radiusY = 0.265f +
                    0.012f * std::sin(fraction * 6.0f * PI);
                const glm::vec3 position(
                    NUCLEUS_CENTER.x + std::cos(angle) * radiusX,
                    NUCLEUS_CENTER.y + std::sin(angle) * radiusY,
                    0.046f
                );
                cloud.addPoint(
                    position,
                    glm::vec4(0.82f, 0.34f, 0.36f, 0.80f)
                );
            }
        }

        void addGolgiApparatus(PointCloud& cloud) {
            // A compact stack of flattened Golgi cisternae, spaced apart so
            // each sac is distinct instead of a dense tangle of loops.
            constexpr int STACKS = 4;
            constexpr int STEPS = 240;
            for (int stack = 0; stack < STACKS; ++stack) {
                const float layer = static_cast<float>(stack);
                for (int step = 0; step < STEPS; ++step) {
                    const float fraction = static_cast<float>(step) /
                        static_cast<float>(STEPS - 1);
                    const float x = (fraction - 0.5f) * 0.34f;
                    const float edge = std::abs(fraction - 0.5f) * 2.0f;
                    const float y = -0.27f - layer * 0.032f +
                        edge * edge * 0.055f;
                    const float z = 0.13f + layer * 0.014f;
                    cloud.addPoint(
                        glm::vec3(x + 0.30f, y, z),
                        glm::vec4(0.48f, 0.16f, 0.39f, 0.80f)
                    );
                }
            }
        }

        void addTransportVesicles(PointCloud& cloud) {
            // Small groups of vesicle points travel slowly along short loops
            // near the Golgi and through the cytoplasm. Their motion is
            // evaluated in the vertex shader, so it adds no per-frame CPU
            // point-cloud rebuild.
            struct VesicleTrack {
                glm::vec3 center;
                float pathRadius;
                float phase;
                float angularSpeed;
            };
            constexpr std::array<VesicleTrack, 5> TRACKS = {{
                {{0.33f, -0.25f, 0.12f}, 0.070f, 0.25f, 0.28f},
                {{0.53f, -0.08f, 0.02f}, 0.052f, 2.10f, -0.22f},
                {{0.43f, 0.27f, -0.12f}, 0.050f, 4.00f, 0.18f},
                {{-0.53f, 0.08f, 0.12f}, 0.046f, 5.20f, -0.25f},
                {{0.10f, -0.42f, -0.10f}, 0.045f, 3.00f, 0.20f}
            }};
            constexpr std::size_t POINTS_PER_VESICLE = 18;

            for (std::size_t trackIndex = 0; trackIndex < TRACKS.size(); ++trackIndex) {
                const VesicleTrack& track = TRACKS[trackIndex];
                const glm::vec3 pathOffset(
                    track.pathRadius * std::cos(track.phase),
                    track.pathRadius * std::sin(track.phase),
                    0.0f
                );

                PointOrbitMotion motion;
                motion.center = track.center;
                motion.axis = glm::vec3(0.0f, 0.0f, 1.0f);
                motion.angularSpeed = track.angularSpeed;

                for (std::size_t pointIndex = 0; pointIndex < POINTS_PER_VESICLE; ++pointIndex) {
                    const glm::vec3 direction = sphereDirection(
                        pointIndex,
                        POINTS_PER_VESICLE
                    );
                    const glm::vec3 position = track.center + pathOffset +
                        direction * 0.012f;
                    const float tint = 0.92f +
                        0.08f * std::sin(
                            static_cast<float>(pointIndex + trackIndex) * 0.7f
                        );
                    cloud.addPoint(
                        position,
                        glm::vec4(0.88f * tint, 0.34f * tint, 0.72f * tint, 0.88f),
                        1.0f,
                        {},
                        motion
                    );
                }
            }
        }

    }

    bool BiologyLesson::recognizesTranscript(
        std::string_view transcript
    ) const {
        const std::string text = lowercase(transcript);
        const bool mentionsCell = text.find("cell") != std::string::npos;
        const bool mentionsCellStructure =
            text.find("nucleus") != std::string::npos ||
            text.find("cytoplasm") != std::string::npos ||
            text.find("organelle") != std::string::npos ||
            text.find("mitochond") != std::string::npos ||
            text.find("cell membrane") != std::string::npos ||
            text.find("basic unit of life") != std::string::npos;
        return mentionsCell && mentionsCellStructure;
    }

    bool BiologyLesson::applyTranscript(std::string_view transcript) {
        if (!recognizesTranscript(transcript) || humanCellVisible_) {
            return false;
        }
        humanCellVisible_ = true;
        return true;
    }

    void BiologyLesson::clear() {
        humanCellVisible_ = false;
    }

    bool BiologyLesson::hasHumanCell() const {
        return humanCellVisible_;
    }

    PointCloud BiologyLesson::buildPointCloud() const {
        PointCloud cloud("Typical Animal Cell");
        if (!humanCellVisible_) {
            return cloud;
        }

        addPlasmaMembrane(cloud);
        addNucleus(cloud);
        addChromatin(cloud);
        addEndoplasmicReticulum(cloud);
        addMitochondrion(
            cloud,
            glm::vec3(-0.40f, -0.25f, 0.045f),
            0.62f
        );
        addMitochondrion(
            cloud,
            glm::vec3(0.39f, 0.25f, -0.035f),
            -0.54f
        );
        addGolgiApparatus(cloud);
        addRibosomes(cloud);
        addTransportVesicles(cloud);

        const std::size_t remainingPoints =
            MORPH_PARTICLE_COUNT > cloud.getPointCount()
                ? MORPH_PARTICLE_COUNT - cloud.getPointCount()
                : 0;
        addCytoplasm(cloud, remainingPoints);
        return cloud;
    }

    std::array<LessonLabel, 8> BiologyLesson::getLabels() const {
        return {{
            {"Cell membrane", glm::vec3(0.70f, 0.40f, 0.00f), {0.12f, 0.84f, 0.98f, 1.0f}},
            {"Cytoplasm", glm::vec3(-0.46f, -0.27f, -0.12f), {0.30f, 0.68f, 0.78f, 1.0f}},
            {"Nucleus", NUCLEUS_CENTER, {0.66f, 0.30f, 0.88f, 1.0f}},
            {"Chromatin (DNA)", NUCLEUS_CENTER + glm::vec3(0.19f, 0.11f, 0.06f), {0.86f, 0.52f, 0.94f, 1.0f}},
            {"Mitochondria", glm::vec3(-0.40f, -0.25f, 0.045f), {1.0f, 0.62f, 0.20f, 1.0f}},
            {"Endoplasmic reticulum", glm::vec3(0.39f, 0.02f, 0.08f), {0.24f, 0.66f, 0.96f, 1.0f}},
            {"Ribosomes", glm::vec3(0.17f, 0.31f, 0.05f), {0.96f, 0.46f, 0.54f, 1.0f}},
            {"Golgi apparatus", glm::vec3(0.30f, -0.37f, 0.15f), {0.98f, 0.46f, 0.76f, 1.0f}}
        }};
    }

}
