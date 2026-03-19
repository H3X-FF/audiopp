#include <vector>
#include <filesystem>

#include "states.h"
#include "ncursesw/ncurses.h"

void initializePlayerState(PlayerState& playerState) {
    playerState.isPlaying = false;
    playerState.shouldRedraw = true;
    playerState.shouldRefreshFiles = false;
    playerState.currSelectionIndex = 0;
    playerState.playingIndex = -1;
    playerState.audioName = "";
    playerState.duration = "";
}

std::vector<fs::path> getAudioFiles() {
    fs::path audioPath{PROJECT_ASSET_DIR};
    audioPath = audioPath / "audio";

    std::vector<fs::path> files;

    if (!fs::is_directory(audioPath)) fs::create_directory(audioPath);

    for (const auto entry : fs::directory_iterator(audioPath)) {
        if (entry.path().extension() == ".wav") {
            files.push_back(entry.path());
        }
    }

    return files;
}

void displayFiles(WINDOW* fileWindow, std::vector<fs::path> audioFiles, PlayerState playerState) {

    int pair;

    for (int i{0}; i < audioFiles.size(); i++) {
        pair = 0;

        if (i == playerState.currSelectionIndex) pair = 1;
        else if (playerState.isPlaying && i == playerState.playingIndex) pair = 2;

        wattron(fileWindow, COLOR_PAIR(pair));
        mvwprintw(fileWindow, i+1, 2, "%s\n", audioFiles[i].filename().c_str());
        wattroff(fileWindow, COLOR_PAIR(pair));
    }
}