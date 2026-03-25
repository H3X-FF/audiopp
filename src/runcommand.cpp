#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <exception>

#include "commandpipeline.hpp"
#include "runcommand.hpp"
#include "ncursesw/curses.h"

namespace fs = std::filesystem;

namespace {
    void addFilesToAppDir(const fs::directory_entry& entry, const fs::path& audioPath, std::string operation) {
        std::string fileExtension{entry.path().extension()};

        // Case-insensitive extension check
        std::transform(fileExtension.begin(), fileExtension.end(),
            fileExtension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });


        if (fileExtension == ".wav" || fileExtension == ".flac" || fileExtension == ".ogg" || fileExtension == ".mp3") {
            fs::copy(entry.path(), audioPath, fs::copy_options::skip_existing);
            if (operation == "--move") fs::remove(entry.path());
        }
    }

    bool isNumber(std::string& str) {
        try {
            //Attempts to convert string to number
            std::stoi(str);

            return true;
        }
        catch (const std::out_of_range& ofr) {
            // incase a number is too large
            return false;
        }
        catch (const std::invalid_argument& ia) {
            // Not a number
            return false;
        }
    }

    void moveFile(int src, std::string& dest, AppState& appState) {
        if (!fs::is_directory(dest)) {
            CommandManager::validate::printError("\'" + dest + "\' doesn't exist");
            return;
        }

        fs::copy(appState.audioFiles[src], dest);
        fs::remove(appState.audioFiles[src]);
    }

    void removeFile(int src, AppState& appState) {
        fs::remove(appState.audioFiles[src]);
    }

    void renameFile(int src, std::string& dest, AppState& appState) {
        fs::path audioppDir{AUDIOPP_PATH};
        fs::rename(appState.audioFiles[src], audioppDir/dest);
    }

    void resolveFile(std::string& src, std::string& dest, std::string operation, AppState& appState) {
        int srcIdx;

        // If the user entered an index instead of a file name
        if (isNumber(src)) {
            srcIdx = std::stoi(src)-1;

            if (srcIdx > appState.numberOfFiles-1 || srcIdx < 0) {
                CommandManager::validate::printError("Out of range index");
                return;
            }

            if (operation == "move") moveFile(srcIdx, dest, appState);
            else if (operation == "remove") removeFile(srcIdx, appState);
            else renameFile(srcIdx, dest, appState);

            appState.shouldRefreshFiles = true;
            appState.shouldRedraw = true;

            return;
        }

        // If user entered a file name, we search for it. Throw an error if file not found
        for (int i{0}; i < appState.numberOfFiles; i++) {
            if (operation == "move") {
                moveFile(i, dest, appState);
                break;
            }

            if (operation == "remove") {
                removeFile(i, appState);
                break;
            }

            if (operation == "rename") { // rename
                renameFile(i, dest, appState);
                break;
            }

            if (i == appState.numberOfFiles - 1) {
                CommandManager::validate::printError("Invalid source: " + src);
                return;
            }
        }

        appState.shouldRefreshFiles = true;
        appState.shouldRedraw = true;
    }

}

void scan(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState) {
    fs::path pathToAudioFiles{args[0]};
    bool isRecursive{false};
    std::string operation{""};

    // Validate source directory
    if (!fs::is_directory(pathToAudioFiles)) {
        CommandManager::validate::printError("Not a directory: " + pathToAudioFiles.string());
        return;
    }

    // Parse command flags
    for (int i{0}; i < flags.size(); i++) {
        if (flags[i] == "--move" || flags[i] =="--copy") operation = flags[i];
        else isRecursive = true;
    }

    fs::path audioPath{AUDIOPP_PATH};

    // Non-recursive file scanning and importation
    if (!isRecursive) {
        for (const auto& entry : fs::directory_iterator(pathToAudioFiles)) {
            addFilesToAppDir(entry, audioPath, operation);
        }
    }
    // Recursive file scanning and importation
    else {
        for (const auto& entry : fs::recursive_directory_iterator(pathToAudioFiles)) {
            addFilesToAppDir(entry, audioPath, operation);
        }
    }

    appState.shouldRefreshFiles = true;
    appState.shouldRedraw = true;
}

void mv(const std::vector<std::string> &args, const std::vector<std::string> &flags, AppState& appState) {
    std::string src{args[0]};
    std::string dst{args[1]};

    resolveFile(src, dst, "move", appState);
}

void rm(const std::vector<std::string> &args, const std::vector<std::string> &flags, AppState &appState) {
    std::string src{args[0]};
    std::string dst{""};

    resolveFile(src, dst, "remove", appState);
}

void rname(const std::vector<std::string> &args, const std::vector<std::string> &flags, AppState &appState) {
    std::string src{args[0]};
    std::string dst{args[1]};

    resolveFile(src, dst, "rename", appState);
}