
#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace chionia {

    class TerminalWindow {
    public:
        TerminalWindow();
        ~TerminalWindow();

        void start();
        void stop();

        bool shouldExit() const;
        void requestExit();

        void setLoadCallback(
            std::function<void(const std::string&)> callback
        );

        void simulateCommand(const std::string& command);

    private:
        void inputLoop();
        void processCommand(const std::string& command);

        std::thread inputThread_;

        std::atomic_bool exitRequested_{false};
        std::atomic_bool threadRunning_{false};

        std::function<void(const std::string&)> loadCallback_;

        std::mutex inputMutex_;
    };

}
