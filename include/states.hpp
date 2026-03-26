#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

/** @brief Represents the synchronization state between the UI and Audio thread. */
enum AudioState {
    RESIZING, // Temporary state during window rebuild
    FAILED,   // Playback initialization error
    SUCCESS,  // Clean exit
    STOPPED,  // User-requested termination
    PLAYING,  // Active playback
    PAUSED,   // Playback suspended
    SEEKING_FWD, // Seeks forward
    SEEKING_BWD // Seeks backward
};

/** @brief Global application state including UI positions and track metadata. */
struct AppState {
    bool isPlaying;
    bool shouldRedraw;
    bool shouldRefreshFiles;
    bool shouldResize;
    bool inCommandMode;
    bool shouldPlayNext;
    bool playPrev;

    std::vector<fs::path> audioFiles;

    int playingIndex;
    int currSelectionIndex;
    int numberOfFiles;

    std::string audioName;
};