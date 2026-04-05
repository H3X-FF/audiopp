#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

/** @brief Represents the synchronization state between the UI and Audio thread. */
enum AudioState {
    FAILED,
    SUCCESS,
    STOPPED,
    PLAYING,
    PAUSED,
    SEEKING_FWD,
    SEEKING_BWD
};

/** @brief State for audio display info, written by AudioManager and read by TUI. */
struct AudioDisplayState {
    std::string audioName;
    std::string progressBar;
    std::string duration;


    int elapsedMinutes;
    int elapsedSeconds;

    double totalSeconds;
    double totalElapsedTime;
    double amplitude;
    double visTimer;
    bool shouldRedraw;
};

/** @brief Global application state including UI positions and track metadata. */
struct AppState {
    bool shouldRedraw;
    bool shouldRefreshFiles;
    bool shouldResize;
    bool inCommandMode;
    bool shouldPlayNext;
    bool shouldPlayPrev;

    std::vector<fs::path> audioFiles;

    int playingIndex;
    int topIndex;
    int currSelectionIndex;
    int numberOfFiles;

    std::string audioName;

    AudioDisplayState audioDisplay;
};

/** @brief Sets default values for the AppState. */
void initializeAppState(AppState& appState);

/** @brief Sets default values for the AudioDisplayState. */
void initializeAudioDisplayState(AudioDisplayState& audioDisplay);

/** @brief Scans the designated audio directory for audio files. */
std::vector<fs::path> getAudioFiles();