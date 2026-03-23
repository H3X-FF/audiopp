#pragma once

#include <atomic>
#include <string>

#include "miniaudio/miniaudio.h"
#include "states.hpp"
#include "ncursesw/ncurses.h"

/**
 * @class AudioManager
 * @brief Handles audio initialization, playback, and TUI info rendering.
 */
class AudioManager {
    ma_engine engine;
    ma_sound sound;

    // Results for error checking during initialization
    ma_result initializingSoundRes;
    ma_result gettingLengthRes;
    ma_result gettingFrameCursorsRes;

    ma_uint64 frameCursor;
    ma_uint32 sampleRate;

    float totalSeconds;
    float remainingSeconds;
    float totalElapsedTime;

    int elapsedMinutes;
    int elapsedSeconds;

    std::string progressBar;
    std::string status;

    /** @brief Converts total seconds into a MM:SS string format. */
    std::string getFullAudioDuration();

    /** @brief Calculates current minutes/seconds from total elapsed time. */
    void formatElapsed();

    /** @brief Updates the ncurses window with current track metadata. */
    void displayAudioInfo(WINDOW** audioInfoWindow, AppState* appState);

    /** @brief Generates a string representation of the playback progress. */
    std::string renderProgressBar(WINDOW** audioInfoWindow);

public:
    /**
     * @brief Primary playback loop intended to run in a separate thread.
     * @param audioState Atomic control for thread communication.
     */
    AudioState playAudio(WINDOW** audioInfoWindow, char* file, std::atomic<AudioState>* audioState, AppState* appState);

    /** @brief Cleanly shuts down the miniaudio engine and sound objects. */
    void uninit();
};