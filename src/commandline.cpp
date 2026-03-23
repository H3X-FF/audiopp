#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <cctype>

#include "commandline.hpp"
#include "states.hpp"
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

void CommandManager::run::scan(const std::vector<std::string>& args, const std::vector<std::string>& flags) {
    fs::path pathToAudioFiles{args[0]};
    bool isRecursive{false};
    std::string operation{""};

    // Validate source directory
    if (!fs::is_directory(pathToAudioFiles)) {
        move(LINES-1, 0);
        clrtoeol();
        attron(COLOR_PAIR(3));
        printw("Not a directory: %s", pathToAudioFiles.c_str());
        attroff(COLOR_PAIR(3));
        return;
    }

    // Parse command flags
    for (int i{0}; i < flags.size(); i++) {
        if (flags[i] == "--move" || flags[i] =="--copy") operation = flags[i];
        else isRecursive = true;
    }

    fs::path audioPath{APP_PATH};
    audioPath /= "audio";

    // Non-recursive file scanning and importation
    if (!isRecursive) {
        for (const auto& entry : fs::directory_iterator(pathToAudioFiles)) {
            std::string fileExtension{entry.path().extension()};

            // Case-insensitive extension check
            std::transform(fileExtension.begin(), fileExtension.end(),
                fileExtension.begin(), [](unsigned char c) {
                return std::tolower(c);
            });

            if (fileExtension == ".wav") {
                fs::copy(entry.path(), audioPath);
                if (operation == "--move") fs::remove(entry.path());
            }
        }
    }
    else {
        for (const auto& entry : fs::recursive_directory_iterator(pathToAudioFiles)) {
            std::string fileExtension{entry.path().extension()};

            // Case-insensitive extension check
            std::transform(fileExtension.begin(), fileExtension.end(),
                fileExtension.begin(), [](unsigned char c) {
                return std::tolower(c);
            });

            if (fileExtension == ".wav") {
                fs::copy(entry.path(), audioPath);
                if (operation == "--move") fs::remove(entry.path());
            }
        }
    }
}

void CommandManager::run::handleCommand(const std::vector<std::string>& tokens) {
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

    // String splitting by space
    std::string currToken;
    std::stringstream stream(prompt);
    std::vector<std::string> tokens;

    while (std::getline(stream, currToken, ' ')) {
        tokens.push_back(currToken);
    }

    std::string command{tokens[0]};
    if (tokens.size() < 2) return;

    // Post-command TUI update triggers
    if (command == "scan") {
        appState.shouldRedraw = true;
        appState.shouldRefreshFiles = true;
    }

    run::handleCommand(tokens);
}