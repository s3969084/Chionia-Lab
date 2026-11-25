#pragma once

#include "scene_command.hpp"

#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace chionia {

    class SceneCommandQueue {
    public:
        void enqueue(SceneCommand command) {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(command));
        }

        std::optional<SceneCommand> tryDequeue() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                return std::nullopt;
            }

            SceneCommand command = std::move(queue_.front());
            queue_.pop();
            return command;
        }

    private:
        std::queue<SceneCommand> queue_;
        std::mutex mutex_;
    };

}
