#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>

#include "runcommand.hpp"
#include "commandpipeline.hpp"

#include <bits/this_thread_sleep.h>

#include "states.hpp"
#include "ncursesw/ncurses.h"

void CommandManager::validate::printError(std::string msg) {
    move(LINES-1, 0);
    clrtoeol();
    attron(COLOR_PAIR(3));
    printw("%s", msg.c_str());
    refresh();

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Temporary (maybe)

    move(LINES-1, 0);
    clrtoeol();
    attroff(COLOR_PAIR(3));

    refresh();
}

void CommandManager::validate::validateCommand(const std::vector<std::string>& tokens) {
    // Registry of available commands and their requirements
    static std::unordered_map<std::string, CommandProperties> commandRegistry{
        {"scan", {1, {"--move", "--copy", "--recurse"}, scan}}
    };

    std::string command{tokens[0]};

    // Validate command existence
    if (commandRegistry.count(command) == 0) {
        printError("Invalid Command: " + command);

        return;
    }

    if (tokens.size() < 2) {
        printError("Not enough arguments");
        return;
    }

    std::vector<std::string> args, flags;

    // Distinguish between positional arguments and optional flags
    for (int i{1}; i < tokens.size(); i++) {
        if (args.size() < commandRegistry[command].minArgs) {
            args.push_back(tokens[i]);
        }
        else if (std::find(commandRegistry[command].allowedFlags.begin(), commandRegistry[command].allowedFlags.end(),
            tokens[i]) != commandRegistry[command].allowedFlags.end()) {
            flags.push_back(tokens[i]);
        }
        else {
            printError("Invalid flag: " + tokens[i]);
            return;
        }
    }

    commandRegistry[command].action(args, flags);
}

void CommandManager::setUpCommand(std::string prompt, AppState& appState) {
    if (prompt.size() < 1) return;

    std::string currToken;
    std::stringstream stream(prompt);
    std::vector<std::string> tokens;

    while (std::getline(stream, currToken, ' ')) {
        tokens.push_back(currToken);
    }

    std::string command{tokens[0]};

    if (command == "scan") {
        appState.shouldRedraw = true;
        appState.shouldRefreshFiles = true;
    }

    validate::validateCommand(tokens);
}