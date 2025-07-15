#pragma once

#include <atomic>
#include <string>
#include <thread>

namespace chionia {

    class TerminalWindow {
    public:
        TerminalWindow();
        ~TerminalWindow();

        void start();           // Launch input thread
        void stop();            // Signal thread to stop and join
        bool shouldExit() const;
        void requestExit();  // Call this when Vulkan window closes

        // Callback to notify Application of a load request
        void setLoadCallback(std::function<void(const std::string&)> callback);

    private:
        void inputLoop();       // Loop for user input

        std::thread inputThread_;
        std::atomic_bool exitRequested_;
        std::atomic_bool threadRunning_;

        std::function<void(const std::string&)> loadCallback_;


    };

}
