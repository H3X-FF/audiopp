#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>

#include "runcommand.hpp"
#include "states.hpp"

namespace CommandManager {

    namespace validate {
        /**
         * @struct CommandProperties
         * @brief Defines the requirements and execution logic for a command.
         */
        struct CommandProperties {
            size_t minArgs;
            std::vector<std::string> allowedFlags;
            std::function<void(const std::vector<std::string>&, const std::vector<std::string>&)> action;
        };

        extern std::thread errorThread;

        void printError(std::string msg);
        /** @brief Validates and executes a command based on parsed tokens. */
        void validateCommand(const std::vector<std::string>& tokens);
    }

    /** @brief Entry point for processing user input from the TUI command line. */
    void setUpCommand(std::string prompt, AppState& appState);
}