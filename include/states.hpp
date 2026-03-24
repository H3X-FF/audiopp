#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

/** @brief Represents the synchronization state between the UI and Audio thread. */
enum AudioState {
    RESIZING, // Temporary state during window rebuild
    FAILED,   // Playback initialization error
    SUCCESS,  // Clean exit
    STOPPED,  // User-requested termination
    PLAYING,  // Active playback
    PAUSED,   // Playback suspended
    FINISHED  // Track reached end
};

/** @brief Global application state including UI positions and track metadata. */
struct AppState {
    bool isPlaying;
    bool shouldRedraw;
    bool shouldRefreshFiles;
    bool shouldResize;
    bool inCommandMode;

    int playingIndex;
    int currSelectionIndex;
    int numberOfFiles;

    std::string audioName;
};

/** @brief Sets default values for the AppState. */
void initializeAppState(AppState& appState);

/** @brief Scans the designated audio directory for .wav files. */
std::vector<fs::path> getAudioFiles();

/** @brief Renders the list of files to the file window with highlighting. */
void displayFiles(WINDOW* fileWindow, std::vector<fs::path> audioFiles, AppState appState);