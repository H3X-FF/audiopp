#pragma once
#ifndef AUDIOPP_AUDIOMANAGER_HPP
#define AUDIOPP_AUDIOMANAGER_HPP

#include <atomic>
#include <string>
#include <thread>
#include <chrono>

#include <miniaudio/miniaudio.h>

#include "states.hpp"

namespace fs = std::filesystem;

/**
 * @class AudioManager
 * @brief Handles audio initialization, playback, and writes display state.
 */
class AudioManager {

    char* audioFile;
    std::atomic<AudioState>* audioState;
    AppState* appState;
    AudioInfoState* audioInfoState;

    std::thread audioThread;

    ma_device device;
    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_decoder_config decoderConfig;

    ma_uint64 frameCursor;
    ma_uint64 totalFrames;
    ma_uint64 frameOffset;


    double totalSeconds;
    double remainingSeconds;
    double totalElapsedTime;
    double deltaTime;

    bool audioFinished;
    bool audioThreadActive;

    std::chrono::time_point<std::chrono::steady_clock> lastBackSeekTime;

    /** @brief A function used by miniaudio's low-level API for delivering real-time PCM data. */
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    /** @brief Converts total seconds into a MM:SS string format. */
    std::string getFullAudioDuration();

    /** @brief Calculates current minutes/seconds from total elapsed time. */
    void formatElapsed();

    /** @brief initializes the devices used by miniaudio. */
    AudioState initializeMA();

    /** @brief Primary thread loop intended to run in a separate thread. */
    void manageAudioThread();

    /** @brief Cleanly shuts down the miniaudio engine and sound objects. */
    void uninitializeMA();


public:
    bool wasPaused;
    std::atomic<float> volumeSlider;

    AudioManager(AudioInfoState* audInfoState, AppState* aState, std::atomic<AudioState>* audioAtomic);

    /** @brief Responsible for creating a new audio thread, and also responsible for stopping an active thread.
     *  Writes audio display info to displayState for the TUI to render. */
    void triggerAudioThread(char* filePath);

    void terminateAudioThread();

    void playNext(std::atomic<AudioState>& audioState, AppState& appState);
    void playPrevious(std::atomic<AudioState> &audioState, AppState &appState);

    ~AudioManager();
};

#endif
