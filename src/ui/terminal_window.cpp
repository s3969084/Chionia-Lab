
#include "terminal_window.hpp"

#include <iostream>
#include <chrono>
#include <utility>

#ifdef _WIN32
    #include <conio.h>
#else
    #include <sys/select.h>
    #include <unistd.h>
#endif

namespace chionia {

    TerminalWindow::TerminalWindow() = default;

    TerminalWindow::~TerminalWindow() {
        stop();
    }

    void TerminalWindow::start() {

        if (inputThread_.joinable()) {
            return;
        }

        exitRequested_ = false;
        threadRunning_ = true;

        inputThread_ = std::thread(&TerminalWindow::inputLoop, this);
    }

    void TerminalWindow::stop() {

        exitRequested_ = true;

        if (inputThread_.joinable()) {
            inputThread_.join();
        }

        threadRunning_ = false;
    }

    bool TerminalWindow::shouldExit() const {
        return exitRequested_.load();
    }

    void TerminalWindow::requestExit() {
        exitRequested_ = true;
    }

    void TerminalWindow::setLoadCallback(
        std::function<void(const std::string&)> callback
    ) {
        std::lock_guard<std::mutex> lock(inputMutex_);
        loadCallback_ = std::move(callback);
    }

    void TerminalWindow::simulateCommand(
        const std::string& command
    ) {
        processCommand(command);
    }

    // ========================================================
    // Command processing
    // ========================================================

    void TerminalWindow::processCommand(
        const std::string& command
    ) {

        if (command == "exit") {

            std::cout << "Terminal thread exiting.\n";
            exitRequested_ = true;
        }
        else if (command.rfind("load ", 0) == 0) {

            std::string filename = command.substr(5);
            std::string fullPath = "data/objects/" + filename;

            std::function<void(const std::string&)> callback;

            {
                std::lock_guard<std::mutex> lock(inputMutex_);
                callback = loadCallback_;
            }

            if (callback) {
                callback(fullPath);
            }
            else {
                std::cout << "No load callback set.\n";
            }
        }
        else if (!command.empty()) {

            std::cout << "Unknown command: " << command << "\n";
        }
    }

    // ========================================================
    // Cross-platform input loop
    // ========================================================

    void TerminalWindow::inputLoop() {

        std::cout
            << "Terminal window initialized. Type 'exit' to quit.\n";

        std::string inputBuffer;

        std::cout << "command >> " << std::flush;

        while (!exitRequested_) {

            char character = '\0';
            bool characterAvailable = false;

#ifdef _WIN32

            // Windows console input
            if (_kbhit()) {

                character = static_cast<char>(_getch());
                characterAvailable = true;

                // Ignore special keys such as arrows and function keys.
                if (character == 0 || character == static_cast<char>(224)) {
                    if (_kbhit()) {
                        _getch();
                    }
                    characterAvailable = false;
                }
            }
            else {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(50)
                );
            }

#else

            // POSIX console input (macOS / Linux)
            fd_set readfds;

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);

            timeval timeout{};
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;

            int result = select(
                STDIN_FILENO + 1,
                &readfds,
                nullptr,
                nullptr,
                &timeout
            );

            if (result > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {

                ssize_t bytesRead = read(
                    STDIN_FILENO,
                    &character,
                    1
                );

                if (bytesRead == 0) {
                    exitRequested_ = true;
                    break;
                }

                if (bytesRead > 0) {
                    characterAvailable = true;
                }
            }
            else if (result < 0) {
                exitRequested_ = true;
                break;
            }

#endif

            if (!characterAvailable) {
                continue;
            }

            // =================================================
            // Process keyboard characters
            // =================================================

            if (character == '\r' || character == '\n') {

#ifdef _WIN32
                std::cout << '\n';
#endif

                processCommand(inputBuffer);

                inputBuffer.clear();

                if (!exitRequested_) {
                    std::cout << "command >> " << std::flush;
                }
            }
            else if (character == '\b' || character == 127) {

                if (!inputBuffer.empty()) {

                    inputBuffer.pop_back();

#ifdef _WIN32
                    std::cout << "\b \b" << std::flush;
#endif
                }
            }
            else if (character == 3) {

                // Ctrl+C
                exitRequested_ = true;
            }
            else if (character >= 32 && character <= 126) {

                inputBuffer += character;

#ifdef _WIN32
                std::cout << character << std::flush;
#endif
            }
        }

        threadRunning_ = false;

        std::cout << "\nTerminal input thread stopped.\n";
    }

}
