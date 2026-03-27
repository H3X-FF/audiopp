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

    ma_device device;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_decoder_config decoderConfig;

    // Results for error checking during initialization
    ma_result decoderInitRes;
    ma_result deviceInitRes;
    ma_result deviceStartRes;

    ma_bool32 isSeekingFwd;

    ma_uint64 frameCursor;
    ma_uint64 totalFrames;
    ma_uint64 frameOffset;

    double totalSeconds;
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

    /** @brief A function used by miniaudio's low-level API for delivering real-time PCM data. */
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    /** @brief Converts total seconds into a MM:SS string format. */
    std::string getFullAudioDuration();

    /** @brief Calculates current minutes/seconds from total elapsed time. */
    void formatElapsed();

    /** @brief Generates a string representation of the playback progress. */
    std::string renderProgressBar(WINDOW** audioInfoWindow);

    /** @brief Renders a smooth traveling oscilloscope line. */
    void renderOscilloscope(WINDOW** audioInfoWindow);

    /** @brief Updates the ncurses window with current track metadata. */
    void displayAudioInfo(WINDOW** audioInfoWindow, AppState* appState);

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