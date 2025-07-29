#include "terminal_window.hpp"
#include <iostream>
#include <unistd.h>     // for STDIN_FILENO
#include <sys/select.h> // for select(), fd_set, timeval

namespace chionia {

    TerminalWindow::TerminalWindow()
        : exitRequested_(false) {}

    TerminalWindow::~TerminalWindow() {
        stop();  // Clean shutdown
    }

    void TerminalWindow::start() {
        inputThread_ = std::thread([this]() {
            std::cout << "Terminal window initialized. Type 'exit' to quit.\n";

            bool promptShown = false;

            while (!exitRequested_) {
                if (!promptShown) {
                    std::cout << "command >> " << std::flush;
                    promptShown = true;
                }

                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(STDIN_FILENO, &readfds);

                timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 100000; // 100ms

                int result = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);

                if (result > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
                    std::string line;
                    if (!std::getline(std::cin, line)) {
                        exitRequested_ = true;
                        break;
                    }

                    if (line == "exit") {
                        std::cout << "✅ Terminal thread exiting.\n";
                        exitRequested_ = true;
                        break;
                    } else if (line.rfind("load ", 0) == 0) {
                        std::string filename = line.substr(5);
                        std::string fullPath = "data/objects/" + filename;

                        if (loadCallback_) {
                            loadCallback_(fullPath);
                        } else {
                            std::cout << "⚠No load callback set.\n";
                        }
                    } else if (!line.empty()) {
                        std::cout << "Unknown command: " << line << "\n";
                    }




                    promptShown = false; // Show new prompt after command processed
                }
            }

            std::cout << "Terminal thread exiting.\n";
        });
    }


    void TerminalWindow::stop() {
        exitRequested_ = true;

        // This hack triggers EOF for blocking std::getline in most environments
        std::cout << "\n";  // Ensure prompt doesn't hang
        std::fflush(stdin); // Try to unblock input in some environments

        if (inputThread_.joinable()) {
            std::cout << "Stopping terminal input thread..." << std::endl;
            inputThread_.join();
        }
    }

    bool TerminalWindow::shouldExit() const {
        return exitRequested_;
    }



    void TerminalWindow::setLoadCallback(std::function<void(const std::string&)> callback) {
        loadCallback_ = std::move(callback);
    }



    void TerminalWindow::requestExit() {
        exitRequested_ = true;
    }

}

