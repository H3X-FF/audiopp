#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

/** @brief Represents the synchronization state between the UI and Audio thread. */
enum class AudioState {
    FAILED,
    SUCCESS,
    STOPPED,
    PLAYING,
    PAUSED,
    SEEKING_FWD,
    SEEKING_BWD,
};

enum class RepeatModes {
    REPEAT_ALL,
    REPEAT_ONE,
    REPEAT_OFF
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

    bool shouldDrawAudioInfo;
    bool shouldRenderAnimation;
    bool displayCurrRepeatMode;
    bool shouldCleanup;

    std::string repeatModeStr;
};

struct SortCommandActions {
    std::string sortType;
    bool reversed;
};

/** @brief Global application state including UI positions and track metadata. */
struct AppState {
    bool shouldRedraw;
    bool shouldRefreshFiles;
    bool shouldCheckForScroll;
    bool shouldResize;
    bool inCommandMode;
    bool shouldPlayNext;
    bool shouldPlayPrev;
    bool shouldChangeRepeatMode;

    std::vector<fs::path> audioFiles;

    RepeatModes repeatMode;

    int playingIndex;
    int topIndex;
    int currSelectionIndex;
    int numberOfFiles;

    AudioDisplayState audioDisplayState;
    SortCommandActions sortActions;
};

/** @brief Sets default values for the AppState. */
void initializeAppState(AppState& appState);

/** @brief Sets default values for the AudioDisplayState. */
void initializeAudioDisplayState(AudioDisplayState& audioDisplay);

/** @brief Scans the designated audio directory for audio files. */
void getAudioFiles(AppState& appState);