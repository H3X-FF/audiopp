#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <functional>
#include <fstream>
#include <iostream>

#include <json/single_include/nlohmann/json.hpp>

using nlohmann::json;
using nlohmann::basic_json;

#include "states.hpp"
#include "file_commands.hpp"

namespace fs = std::filesystem;


namespace {
    void addFilesToAppDir(const fs::directory_entry& entry, const fs::path& audioPath, json& playlistObj) {
        std::string fileExtension = entry.path().extension().u8string();

        // Case-insensitive extension check
        std::transform(fileExtension.begin(), fileExtension.end(),
            fileExtension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });


        if (fileExtension == ".wav" || fileExtension == ".flac" || fileExtension == ".mp3") {
            playlistObj[entry.path().filename().u8string()] = entry.path().u8string();
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

    void removeFile(std::string& file, AppState& appState) {
        std::ifstream ifs(appState.audioppJsonFile);
        
        if (ifs.good()) {
            
            try {
                json j = json::parse(ifs);
                ifs.close();

                if (j.contains(appState.vfs.currPlaylist)) {
                    auto& playlistObj = j[appState.vfs.currPlaylist];
                    playlistObj.erase(file);

                    std::ofstream ofs(appState.audioppJsonFile, std::ios::trunc);

                    ofs << j.dump(4);

                    ofs.close();
                }
            }
            catch (json::exception& e) {
                printError("Failed to remove file!", appState);
            }
        }
    }


    void renameFile(std::string& oldName, std::string& newName, AppState& appState) {
        // fs::path audioppDir = AUDIOPP_PATH;

        fs::path oldNameExt = oldName;
        fs::path newNameExt = newName;

        if (oldNameExt.extension() != newNameExt.extension()) {
            newName += oldNameExt.extension().u8string();
        }

        std::ifstream ifs(appState.audioppJsonFile);

        if (ifs.good()) {
            try {
                json j = json::parse(ifs);
                ifs.close();

                if (j.contains(appState.vfs.currPlaylist)) {
                    auto& playlistObj = j[appState.vfs.currPlaylist];

                    if (playlistObj.contains(oldName)) {

                        // Perform the rename
                        std::swap(playlistObj[newName], playlistObj[oldName]);
                        playlistObj.erase(oldName);

                        std::ofstream ofs(appState.audioppJsonFile, std::ios::trunc);
                        ofs << j.dump(4);

                        ofs.close();
                    }
                }
            }
            catch (const json::exception& e) {
                printError("Failed to rename file!", appState);
            }
        }
    }

    void resolveFile(std::string& src, std::string& dst, AppState& appState, std::function<void()> action) {
        // If the user entered an index instead of a file name
        if (isNumber(src)) {
            int srcIdx = std::stoi(src)-1;

            if (srcIdx > appState.numberOfFiles-1 || srcIdx < 0) {
                printError("Out of range index", appState);
                return;
            }

            if (srcIdx == appState.playingIndex) {
                printError("Can't modify an active track", appState);
                return;
            }

            src = appState.vfs.audioFileNames[srcIdx];

            // Triggers the respective action if it's removing a file or renaming it
            action();

            appState.shouldRefreshFiles = true;

            return;
        }

        // If user entered a file name, we search for it. Throw an error if file not found
        for (int i{0}; i < appState.numberOfFiles; i++) {

            if (appState.vfs.audioFileNames[i] == src) {

                if (i == appState.playingIndex) {
                    printError("Can't modify an active track", appState);
                    return;
                }

                // Triggers the respective action if it's removing a file or renaming it
                src = appState.vfs.audioFileNames[i];
                action();
                break;
            }

            if (i == appState.numberOfFiles - 1) {
                printError("Invalid source file: " + src, appState);
                return;
            }

        }

        appState.shouldRefreshFiles = true;
    }

} // namespace

void scan(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState) {
    fs::path pathToAudioFiles{args[0]};
    bool isRecursive{!flags.empty() && flags[0] == "--recurse"};

    // Validate source directory
    if (!fs::is_directory(pathToAudioFiles)) {
        printError("Not a directory: " + pathToAudioFiles.u8string(), appState);
        return;
    }

    json j;

    std::ifstream ifs(appState.audioppJsonFile);

    if (ifs.is_open()) {
        try {
            if (ifs.good()) {
                j = json::parse(ifs);
            }

            ifs.close();
        }
        catch (json::exception& e) {
            printError("Failed to parse existing library!", appState);
            return;
        }

        ifs.close();
    }
    try {
        // Non-recursive file scanning and importation
        if (!isRecursive) {
            for (const auto& entry : fs::directory_iterator(pathToAudioFiles)) {
                addFilesToAppDir(entry, appState.audioppPath, j[appState.vfs.currPlaylist]);
            }
        }
        // Recursive file scanning and importation
        else {
            for (const auto& entry : fs::recursive_directory_iterator(pathToAudioFiles)) {
                addFilesToAppDir(entry, appState.audioppPath, j[appState.vfs.currPlaylist]);
            }
        }

        std::ofstream ofs(appState.audioppJsonFile, std::ios::trunc);
        if (ofs.is_open()) {
            ofs << j.dump(4);
            ofs.close();
            appState.shouldRefreshFiles = true;
        }
        else {
            printError("Failed to write files to library!", appState);
        }
    }
    catch (const json::exception& e) {
        printError("scan failed! Try again later", appState);
    }
    catch (const fs::filesystem_error& fe) {
        printError("Filesystem error during scan: " + std::string(fe.what()), appState);
	}
    catch(const std::exception& ex) {
        printError("An unexpected error occurred during scan: " + std::string(ex.what()), appState);
	}

}

void rm(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState) {
    std::string src{args[0]};
    std::string dst{""};

    resolveFile(src, dst, appState, [&]() {
        removeFile(src, appState);
    });
}

void rname(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState) {
    std::string src{args[0]};
    std::string dst{args[1]};

    resolveFile(src, dst, appState, [&]() {
        renameFile(src, dst, appState);
    });
}