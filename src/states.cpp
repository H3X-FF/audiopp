#include "states.h"
#include "ncurses/ncurses.h"

void initializePlayerState(PlayerState& playerState) {
    playerState.isPlaying = false;
    playerState.shouldRedraw = true;
    playerState.shouldRefreshFiles = false;
    playerState.currSelectionIndex = 0;
    playerState.playingIndex = -1;
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

void displayFiles(std::vector<fs::path> audioFiles, PlayerState playerState) {
    move(0, 0);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);

    int pair;
    for (int i{0}; i < audioFiles.size(); i++) {
        pair = 0;

        if (i == playerState.currSelectionIndex) pair = 1;
        else if (playerState.isPlaying && i == playerState.playingIndex) pair = 2;
        else if (!playerState.isPlaying && i == playerState.playingIndex) pair = 3;

        attron(COLOR_PAIR(pair));
        printw("%s\n", audioFiles[i].filename().c_str());
        attroff(COLOR_PAIR(pair));
    }
}