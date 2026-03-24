#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "runcommand.hpp"
#include "commandpipeline.hpp"
#include "states.hpp"
#include "ncursesw/ncurses.h"


void CommandManager::validate::handleCommand(const std::vector<std::string>& tokens) {
    // Registry of available commands and their requirements
    static std::unordered_map<std::string, CommandProperties> commandRegistry{
        {"scan", {1, {"--move", "--copy", "--recurse"}, scan}}
    };

    std::string command{tokens[0]};

    // Validate command existence
    if (commandRegistry.count(command) == 0) {
        move(LINES-1, 0);
        clrtoeol();
        attron(COLOR_PAIR(3));
        printw("Invalid command: %s", command.c_str());
        attroff(COLOR_PAIR(3));
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
            move(LINES-1, 0);
            clrtoeol();
            attron(COLOR_PAIR(3));
            printw("Invalid flag: %s", tokens[i].c_str());
            attroff(COLOR_PAIR(3));
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
    if (tokens.size() < 2) return;

    if (command == "scan") {
        appState.shouldRedraw = true;
        appState.shouldRefreshFiles = true;
    }

    validate::handleCommand(tokens);
}