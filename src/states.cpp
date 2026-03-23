#include <vector>
#include <filesystem>
#include <cstdlib>
#include <string>

#include "states.hpp"
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

void initializeAppState(AppState& appState) {
    appState.isPlaying = false;
    appState.shouldRedraw = true;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;

    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;

    appState.audioName = "";
    appState.duration = "";
}

std::vector<fs::path> getAudioFiles() {
    fs::path audioPath{APP_PATH};
    audioPath /= "audio";

    std::vector<fs::path> files;

    // Ensure storage directory exists
    if (!fs::exists(audioPath)) fs::create_directories(audioPath);

    // Populate file list with supported formats
    for (const auto entry : fs::directory_iterator(audioPath)) {
        if (entry.path().extension() == ".wav") {
            files.push_back(entry.path());
        }
    }

    return files;
}

void displayFiles(WINDOW* fileWindow, std::vector<fs::path> audioFiles, AppState appState) {
    int pair;

    for (int i{0}; i < audioFiles.size(); i++) {
        pair = 0; // Default style

        // Apply highlighting for current cursor selection or currently playing file
        if (i == appState.currSelectionIndex) pair = 1;
        else if (appState.isPlaying && i == appState.playingIndex) pair = 2;

        wattron(fileWindow, COLOR_PAIR(pair));
        mvwprintw(fileWindow, i+1, 2, "%s\n", audioFiles[i].filename().c_str());
        wattroff(fileWindow, COLOR_PAIR(pair));
    }
}