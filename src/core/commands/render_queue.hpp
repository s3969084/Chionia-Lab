#pragma once

#include "render_command.hpp"
#include <queue>
#include <mutex>
#include <optional>

namespace chionia {

    class RenderQueue {
    public:
        void enqueue(const RenderCommand& cmd) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(cmd);
        }

        std::optional<RenderCommand> tryDequeue() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return std::nullopt;
            RenderCommand cmd = queue_.front();
            queue_.pop();
            return cmd;
        }

    private:
        std::queue<RenderCommand> queue_;
        std::mutex mutex_;
    };


}

