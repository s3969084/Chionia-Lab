#pragma once

#include "render_command.hpp"
#include <queue>
#include <mutex>
#include <optional>
#include <utility>

namespace chionia {

    class RenderQueue {
    public:
        void enqueue(RenderCommand cmd) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(cmd));
        }

        std::optional<RenderCommand> tryDequeue() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return std::nullopt;
            RenderCommand cmd = std::move(queue_.front());
            queue_.pop();
            return cmd;
        }

    private:
        std::queue<RenderCommand> queue_;
        std::mutex mutex_;
    };


}

