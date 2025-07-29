#pragma once

#include <vector>
#include "../resources/vertex.hpp"
#include "scene/camera.hpp"

namespace chionia {

    class IRenderBackend {
    public:
        virtual void init() = 0;
        virtual void drawFrame(const Camera& camera) = 0;
        virtual void updateVertices(const std::vector<Vertex>& vertices) = 0;
        virtual void reloadPipeline() = 0;
        virtual void cleanup() = 0;
        virtual ~IRenderBackend() = default;

        virtual bool shouldClose() const = 0;

    };
}