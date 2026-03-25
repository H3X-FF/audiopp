#include <vector>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "states.hpp"
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

void initializeAppState(AppState& appState) {
    appState.isPlaying = false;
    appState.shouldRedraw = true;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;
    appState.inCommandMode = false;
    appState.playNext = false;

    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;

    appState.audioName = "";
}

std::vector<fs::path> getAudioFiles() {
    fs::path audioPath{AUDIOPP_PATH}; // AUDIOPP_PATH is defined in the CMakeLists.txt

    std::vector<fs::path> files;

    if (!fs::exists(audioPath)) fs::create_directory(audioPath);

    for (const auto entry : fs::directory_iterator(audioPath)) {
        std::string fileExtension{entry.path().extension()};

        // Case-insensitive extension check
        std::transform(fileExtension.begin(), fileExtension.end(),
            fileExtension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (fileExtension == ".wav" || fileExtension == ".flac" || fileExtension == ".ogg" || fileExtension == ".mp3") {
            files.push_back(entry.path());
        }
    }

    return files;
}

void displayFiles(WINDOW* fileWindow, std::vector<fs::path> audioFiles, AppState appState) {
    int pair;

    for (int i{0}; i < audioFiles.size(); i++) {
        pair = 0;

        if (i == appState.currSelectionIndex) pair = 1;
        else if (appState.isPlaying && i == appState.playingIndex) pair = 2;

        wattron(fileWindow, COLOR_PAIR(pair));
        mvwprintw(fileWindow, i+1, 2, "%d. %s\n", i+1, audioFiles[i].filename().c_str());
        wattroff(fileWindow, COLOR_PAIR(pair));
    }
}