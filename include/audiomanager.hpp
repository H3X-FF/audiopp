#pragma once

#include <atomic>
#include <string>
#include <thread>

#include "miniaudio/miniaudio.h"
#include "states.hpp"
#include "ncursesw/ncurses.h"

/**
 * @class AudioManager
 * @brief Handles audio initialization, playback, and TUI info rendering.
 */
class AudioManager {
    WINDOW** audioInfoWindow;
    char* audioFile;
    std::atomic<AudioState>* audioState;
    AppState* appState;

    std::thread audioThread;

    ma_engine engine;
    ma_sound sound;

    // Results for error checking during initialization
    ma_result initializingSoundRes;
    ma_result gettingLengthRes;
    ma_result gettingFrameCursorsRes;

    ma_uint32 sampleRate;
    ma_uint64 frameCursor;
    ma_uint64 totalFrames;
    ma_uint64 frameOffset;
    ma_uint64 seekingPos;

    float totalSeconds;
    double remainingSeconds;
    double totalElapsedTime;

    int elapsedMinutes;
    int elapsedSeconds;

    int maxAmplitude;
    double amplitude;
    double visTimer;

    std::string duration;
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

    /** @brief Renders a smooth traveling oscilloscope line. */
    void renderOscilloscope(WINDOW** audioInfoWindow);

    /** @brief initializes the devices used by miniaudio. */
    AudioState initializeMA();

    /**
     * @brief Primary playback loop intended to run in a separate thread.
     * @param audioState Atomic control for thread communication.
     */
    void playAndManageAudio();

    /** @brief Cleanly shuts down the miniaudio engine and sound objects. */
    void uninitializeMA();


public:
    AudioManager();

    /** @brief Responsible for creating a new audio thread, and also responsible for stopping an active thread */
    void triggerAudioThread(WINDOW** infoWin, AppState* aState,
                                    std::atomic<AudioState>* audioAtomic, char* filePath);

    void terminateAudioThread();
};