#ifndef AUDIOPP_GET_STATE_H
#define AUDIOPP_GET_STATE_H

#include <filesystem>
#include <vector>
#include <string>
#include <atomic>

namespace fs = std::filesystem;

enum AudioState {
    STOPPED,
    PLAYING
};

struct PlayerState {
    bool isPlaying;
    bool shouldRedraw;
    bool shouldRefreshFiles;
    int playingIndex;
    int currSelectionIndex;
    std::string audioName;
};

void initializePlayerState(PlayerState& playerState);

std::vector<fs::path> getAudioFiles();
void displayFiles(std::vector<fs::path> audioFiles, PlayerState playerState);

#endif //AUDIOPP_GET_STATE_H