#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <chrono>

#include <miniaudio/miniaudio.h>

#include "states.hpp"

/**
 * @class AudioManager
 * @brief Handles audio initialization, playback, and writes display state.
 */
class AudioManager {
    char* audioFile;
    std::atomic<AudioState>* audioState;
    AppState* appState;
    AudioDisplayState* displayState;

    std::thread audioThread;

    ma_device device;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_decoder_config decoderConfig;

    // Results for error checking during initialization
    ma_result decoderInitRes;
    ma_result deviceInitRes;
    ma_result deviceStartRes;

    ma_uint64 frameCursor;
    ma_uint64 totalFrames;
    ma_uint64 frameOffset;

    double totalSeconds;
    double remainingSeconds;
    double totalElapsedTime;

    bool audioFinished;

    std::chrono::time_point<std::chrono::steady_clock> lastBackSeekTime;

    /** @brief A function used by miniaudio's low-level API for delivering real-time PCM data. */
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    /** @brief Converts total seconds into a MM:SS string format. */
    std::string getFullAudioDuration();

    /** @brief Calculates current minutes/seconds from total elapsed time. */
    void formatElapsed();

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
    bool pausedWhileSeeking;

    /** @brief Responsible for creating a new audio thread, and also responsible for stopping an active thread.
     *  Writes audio display info to displayState for the TUI to render. */
    void triggerAudioThread(AudioDisplayState* dState, AppState* aState,
                                    std::atomic<AudioState>* audioAtomic, char* filePath);

    void terminateAudioThread();

    void playNext(std::atomic<AudioState>& audioState, AppState& appState);
    void playPrevious(std::atomic<AudioState> &audioState, AppState &appState);
};