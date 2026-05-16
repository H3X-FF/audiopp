#pragma once
#ifndef AUDIOPP_STATES_HPP
#define AUDIOPP_STATES_HPP

#include <filesystem>
#include <vector>
#include <string>
#include <array>
#include <atomic>
#include <unordered_map>

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
struct AudioInfoState {
    std::string audioName;
    std::string duration;
    std::string repeatModeInfo;


    int elapsedMinutes;
    int elapsedSeconds;

    double totalSeconds;
    double totalElapsedTime;

    size_t bufWriteIdx;
    std::array<float, 1024> samplesBuf;
    std::atomic<bool> samplesReady;
    std::atomic<bool> visThreadShouldExit;

    float volume;

    bool shouldDrawAudioInfo;
    bool shouldRenderAnimation;
    bool shouldUpdateVolOrRepeatTxt;
    bool shouldCleanup;
};

struct SortCommandActions {
    std::string sortType;
    bool reversed;
};

struct VirtualFS {
    // Will be used for the future when playlists are added. For now it's used for "main":
    std::string currPlaylist;
    std::unordered_map<std::string, std::string> audioMap;
    std::vector<std::string> audioFileNames;
};

/** @brief Global application state including UI positions and track metadata. */
struct AppState {
    bool shouldRedrawScreen;
    bool shouldRedrawFileList;
    bool shouldRefreshFiles;
    bool shouldCheckForScroll;
    bool shouldResize;
    bool inCommandMode;
    bool shouldPlayNext;
    bool shouldPlayPrev;

    RepeatModes repeatMode;

    int playingIndex;
    int topIndex;
    int currSelectionIndex;
    int numberOfFiles;
    int errorTick;

    fs::path audioppPath;
    fs::path audioppJsonFile;
    fs::path audioppSettingsFile;

    AudioInfoState audioInfoState;
    SortCommandActions sortActions;
    VirtualFS vfs;
};

void initializeVfs(VirtualFS& vfs);

/** @brief Sets default values for the AppState. */
void initializeAppState(AppState& appState);

/** @brief Sets default values for the AudioDisplayState. */
void initializeAudioDisplayState(AudioInfoState& audioDisplay);

void printError(std::string msg, AppState& appState);

#endif