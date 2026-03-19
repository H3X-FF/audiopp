#ifndef AUDIOPP_GET_STATE_H
#define AUDIOPP_GET_STATE_H

#include <filesystem>
#include <vector>
#include <string>
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

// Used in std::atomic to manage the audio playing thread
enum AudioState {
    STOPPED,
    PLAYING,
    PAUSED
};

struct PlayerState {
    bool isPlaying;
    bool shouldRedraw;
    bool shouldRefreshFiles;
    int playingIndex;
    int currSelectionIndex;
    std::string audioName;
    std::string duration;
    std::string status;
};

void initializePlayerState(PlayerState& playerState);

std::vector<fs::path> getAudioFiles();
void displayFiles(WINDOW* fileWindow, std::vector<fs::path> audioFiles, PlayerState playerState);

#endif //AUDIOPP_GET_STATE_H