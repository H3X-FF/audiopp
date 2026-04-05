#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <functional>

#include "commandpipeline.hpp"
#include "runcommand.hpp"
namespace fs = std::filesystem;

namespace {
    void addFilesToAppDir(const fs::directory_entry& entry, const fs::path& audioPath, bool shouldMove) {
        std::string fileExtension{entry.path().extension()};

        // Case-insensitive extension check
        std::transform(fileExtension.begin(), fileExtension.end(),
            fileExtension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        static int deniedFiles = 0;

        if (fileExtension == ".wav" || fileExtension == ".flac" || fileExtension == ".mp3") {
            fs::copy(entry.path(), audioPath, fs::copy_options::skip_existing);

            // Removing the file from the original path for the --move option
            if (shouldMove) {
                std::error_code ec;

                fs::remove(entry.path(), ec);

                if (ec == std::errc::permission_denied) {
                    deniedFiles++;

                    CommandManager::validate::printError(
                        "Permission denied for attempting to move " + std::to_string(deniedFiles) +
                        " files. Copied instead"
                        );

                }
            }
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

//----------------------------------------------------------------------------------------------------------------------
    void removeFile(std::string& src, AppState& appState) {
        fs::remove(src);
    }

    void renameFile(std::string& src, std::string& dst, AppState& appState) {
        fs::path filePath = src; // just to get the extension :P
        std::string extension = filePath.extension();

        fs::path audioppDir{AUDIOPP_PATH};
        fs::rename(src, audioppDir/(dst + extension));
    }

    void resolveFile(std::string& src, std::string& dst, AppState& appState, std::function<void()> action) {
        // If the user entered an index instead of a file name
        if (isNumber(src)) {
            int srcIdx = std::stoi(src)-1;

            if (srcIdx > appState.numberOfFiles-1 || srcIdx < 0) {
                CommandManager::validate::printError("Out of range index");
                return;
            }

            src = appState.audioFiles[srcIdx];

            // Triggers the respective action if it's removing a file or renaming it
            action();

            appState.shouldRefreshFiles = true;

            return;
        }

        // If user entered a file name, we search for it. Throw an error if file not found
        for (int i{0}; i < appState.numberOfFiles; i++) {

            if (appState.audioFiles[i].filename() == src) {
                // Triggers the respective action if it's removing a file or renaming it
                action();
                break;
            }

            if (i == appState.numberOfFiles - 1) {
                CommandManager::validate::printError("Invalid source file: " + src);
                return;
            }

        }

        appState.shouldRefreshFiles = true;
    }

//----------------------------------------------------------------------------------------------------------------------

}

void scan(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState) {
    fs::path pathToAudioFiles{args[0]};
    bool isRecursive{false};

    // Validate source directory
    if (!fs::is_directory(pathToAudioFiles)) {
        CommandManager::validate::printError("Not a directory: " + pathToAudioFiles.string());
        return;
    }

    bool shouldMove = false;

    // Parse command flags
    for (int i{0}; i < flags.size(); i++) {
        if (flags[i] == "--move") shouldMove = true;
        else if (flags[i] == "--copy") shouldMove = false;

        if (flags[i] == "--recurse") isRecursive = true;
    }

    fs::path audioPath{AUDIOPP_PATH};

    // Non-recursive file scanning and importation
    if (!isRecursive) {
        for (const auto& entry : fs::directory_iterator(pathToAudioFiles)) {
            addFilesToAppDir(entry, audioPath, shouldMove);
        }
    }
    // Recursive file scanning and importation
    else {
        for (const auto& entry : fs::recursive_directory_iterator(pathToAudioFiles)) {
            addFilesToAppDir(entry, audioPath, shouldMove);
        }
    }

    appState.shouldRefreshFiles = true;
}

void rm(const std::vector<std::string> &args, const std::vector<std::string> &flags, AppState &appState) {
    std::string src{args[0]};
    std::string dst{""};

    resolveFile(src, dst, appState, [&]() {
        removeFile(src, appState);
    });
}

void rname(const std::vector<std::string> &args, const std::vector<std::string> &flags, AppState &appState) {
    std::string src{args[0]};
    std::string dst{args[1]};

    resolveFile(src, dst, appState, [&]() {
        renameFile(src, dst, appState);
    });
}