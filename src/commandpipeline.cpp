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

void CommandManager::validate::validateCommand(const std::vector<std::string>& tokens, AppState& appState) {
    // Registry of available commands and their requirements
    static std::unordered_map<std::string, CommandProperties> commandRegistry{
        {"scan", {1, {"--move", "--copy", "--recurse"}, scan}},
        {"mv", {2, {}, mv}},
        {"rm", {1, {}, rm}},
        {"rename", {2, {}, rname}}
    };

    std::string command{tokens[0]};

    // Validate command existence
    if (commandRegistry.count(command) == 0) {
        printError("Invalid Command: " + command);

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
            printError("Invalid arguments: " + tokens[i]);
            return;
        }
    }

    if (args.size() < commandRegistry[command].minArgs) {
        printError("Not enough arguments");
        return;
    }

    commandRegistry[command].action(args, flags, appState);
}

void CommandManager::setUpCommand(std::string prompt, AppState& appState) {
    if (prompt.size() < 1) return;

    std::string currToken;
    std::stringstream stream(prompt);
    std::vector<std::string> tokens;

    while (std::getline(stream, currToken, ' ')) {
        tokens.push_back(currToken);
    }

    validate::validateCommand(tokens, appState);
}