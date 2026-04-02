#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include "audiomanager.hpp"
#include "states.hpp"

void AudioManager::terminateAudioThread() {
    audioState->store(STOPPED);
    if (audioThread.joinable()) audioThread.join();
}
// Resets UI-related playback state when audio stops.
void uninitializeAppState(AppState* appState, AudioDisplayState* displayState) {
    appState->playingIndex = -1;
    appState->audioName = "";

    displayState->shouldRedraw = true;
}

std::string AudioManager::getFullAudioDuration() {
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss<< minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);
    return ss.str();
}

void AudioManager::formatElapsed() {
    int elapsedMinutes = static_cast<int>(totalElapsedTime) / 60;
    int elapsedSeconds = static_cast<int>(totalElapsedTime) % 60;

    displayState->elapsedMinutes = elapsedMinutes;
    displayState->elapsedSeconds = elapsedSeconds;
}

/*
* Plays audio and sets up the audio thread. It signals a stop first, checks if we can join the thread
* so that if there's an active thread that thread exits the loop, resets states
* then the new thread comes in and plays the new audio. */
void AudioManager::triggerAudioThread(AudioDisplayState* dState, AppState* aState, std::atomic<AudioState>* audioAtomic, char* filePath) {
    displayState = dState;
    appState = aState;
    audioState = audioAtomic;
    audioFile = filePath;

    audioState->store(STOPPED);

    if (audioThread.joinable()) audioThread.join();

    audioThread = std::thread(&AudioManager::playAndManageAudio, this);
}

void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioManager* pManager{static_cast<AudioManager*>(pDevice->pUserData)};
    AudioState currentState = pManager->audioState->load();


    if (currentState == PAUSED) {
        // Clearing the buffer here for miniaudio to continue reading data but without playing the actual audio.
        // Reason for this approach to pause is just to allow seeking while paused. Using ma_device_stop() stops data_callback
        // which ends up blocking seeking while paused.
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame);
        return;
    }

    // If total frames were to be zero, then that means the file hasn't loaded yet
    if (currentState == SEEKING_FWD && pManager->totalFrames != 0) {
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame); // Clearing the buffer to avoid a weird glitch sound

        ma_uint64 newPos = pManager->frameCursor + pManager->frameOffset;

        if (newPos >= pManager->totalFrames) {
            pManager->appState->shouldPlayNext = true;
            pManager->audioState->store(STOPPED);
            return;
        }

        ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);
        pManager->audioState->store(pManager->pausedWhileSeeking ? PAUSED : PLAYING);
        pManager->pausedWhileSeeking = false;
    }

    // If total frames were to be zero, then that means the file hasn't loaded yet
    if (currentState == SEEKING_BWD && pManager->totalFrames != 0) {
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame); // Clearing the buffer to avoid a weird glitch sound

        if (pManager->frameCursor <= pManager->frameOffset) {
            // We are within the first 5 seconds, trigger Previous Track
            pManager->appState->shouldPlayPrev = true;
            pManager->audioState->store(STOPPED); // Signal loop to exit
            return; // Don't read frames when at start
        }

        ma_uint64 newPos = pManager->frameCursor - pManager->frameOffset;
        ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);
        pManager->audioState->store(pManager->pausedWhileSeeking ? PAUSED : PLAYING);
        pManager->pausedWhileSeeking = false;

    }

    // Only read frames if not stopped
    if (pManager->audioState->load() == PLAYING) {
        ma_decoder_read_pcm_frames(&pManager->decoder, pOutput, frameCount, NULL);
    }
}

// Initializes miniaudio. After initializing miniaudio, it will set the state to playing.
AudioState AudioManager::initializeMA() {

    decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    decoderConfig.seekPointCount = 128; // Helps with a smoother seeking especially with mp3s

    decoderInitRes = ma_decoder_init_file(audioFile, &decoderConfig, &decoder);

    if (decoderInitRes != MA_SUCCESS) {
        audioState->store(STOPPED);
        return FAILED;
    }

    // Reset playback position for new file
    frameCursor = 0;
    totalFrames = 0;

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = this;


    deviceInitRes = ma_device_init(NULL, &deviceConfig, &device);

    if (deviceInitRes != MA_SUCCESS) {
        audioState->store(STOPPED);
        ma_decoder_uninit(&decoder);
        return FAILED;
    }

    deviceStartRes = ma_device_start(&device);

    if (deviceStartRes != MA_SUCCESS) {
        audioState->store(STOPPED);
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return FAILED;
    }

    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    totalSeconds = static_cast<double>(totalFrames) / deviceConfig.sampleRate;

    totalElapsedTime = 0;

    frameOffset = 5 * deviceConfig.sampleRate; // 5 seconds in frame

    pausedWhileSeeking = false;
    audioState->store(PLAYING);

    displayState->audioName = appState->audioName;
    displayState->duration = getFullAudioDuration();
    displayState->shouldRedraw = true;

    return SUCCESS;
}

void AudioManager::playAndManageAudio() {
    if (initializeMA() == FAILED) return;

    frameCursor = 0;
    double amplitude = 0.0;
    double visTimer = 0.0;

    auto lastTime{std::chrono::high_resolution_clock::now()};

    // The audio thread settles in here. It performs action depending on the audio state.
    while (audioState->load() != STOPPED && frameCursor < totalFrames) {
        auto currentTime{std::chrono::high_resolution_clock::now()};
        double dt{std::chrono::duration<double>(currentTime - lastTime).count()};
        lastTime = currentTime;

        ma_decoder_get_cursor_in_pcm_frames(&decoder, &frameCursor);

        totalElapsedTime = static_cast<double>(frameCursor) / deviceConfig.sampleRate;

        // Increment timer for wave movement
        visTimer += 5.0 * dt;

        // Handling the visualizer by making a smooth fade in/fade out depending on state
        if (audioState->load() == PLAYING) {
            ma_device_start(&device);
            if (amplitude < 1.0) amplitude += 4.0 * dt;
        }
        else if (audioState->load() == PAUSED) {
            // NOTE: The actual pause happens in data_callback()
            if (amplitude > 0) amplitude -= 4.0 * dt;
        }

        // Update display state (but skip during resize to avoid flickering)
        if (audioState->load() != RESIZING) {
            formatElapsed();
            displayState->totalSeconds = totalSeconds;
            displayState->totalElapsedTime = totalElapsedTime;
            displayState->amplitude = amplitude;
            displayState->visTimer = visTimer;
            displayState->shouldRedraw = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    if (frameCursor >= totalFrames && !appState->shouldPlayPrev && !appState->shouldPlayNext) appState->shouldPlayNext = true;
    else if (!appState->shouldPlayNext && !appState->shouldPlayPrev) uninitializeAppState(appState, displayState);

    uninitializeMA();
}

void AudioManager::uninitializeMA() {
    ma_device_stop(&device);
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}

void AudioManager::playNext(std::atomic<AudioState>& audioState, AppState& appState) {
    if (appState.playingIndex < appState.numberOfFiles - 1) appState.playingIndex++;
    else appState.playingIndex = 0;

    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.playingIndex].c_str())};
    this->triggerAudioThread(&appState.audioDisplay, &appState, &audioState, audioFilePath);


    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
    appState.shouldRedraw = true;

    appState.shouldPlayNext = false;
}

void AudioManager::playPrevious(std::atomic<AudioState> &audioState, AppState &appState) {
    if (appState.playingIndex > 0) appState.playingIndex--;
    else appState.playingIndex = appState.numberOfFiles-1;

    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.playingIndex].c_str())};
    this->triggerAudioThread(&appState.audioDisplay, &appState, &audioState, audioFilePath);

    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
    appState.shouldRedraw = true;

    appState.shouldPlayPrev = false;
}