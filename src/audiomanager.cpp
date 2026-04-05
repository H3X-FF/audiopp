#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

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
    // Convert total seconds into an MM:SS format string
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss << minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);
    return ss.str();
}

void AudioManager::formatElapsed() {
    // Break down total elapsed time into minutes and seconds for the UI
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

    // Wait for the existing thread to finish its cleanup before starting a new one
    if (audioThread.joinable()) audioThread.join();

    audioThread = std::thread(&AudioManager::playAndManageAudio, this);
}

void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioManager* pManager{static_cast<AudioManager*>(pDevice->pUserData)};
    AudioState currentState = pManager->audioState->load();

    if (currentState == PAUSED) {
        /* Clearing the buffer here for miniaudio to continue reading data but without playing the actual audio.
         * Reason for this approach for pausing is just to allow seeking while paused.
         * Using ma_device_stop() stops data_callback() which ends up blocking seeking while the audio is paused. */
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame);
        return;
    }

    // If total frames were to be zero, then that means the file hasn't loaded yet
    if (currentState == SEEKING_FWD && pManager->totalFrames != 0) {
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame); // Clearing the buffer to avoid a weird glitch sound

        ma_uint64 newPos = pManager->frameCursor + pManager->frameOffset;

        // If seeking forward goes beyond the track length, trigger the next song
        if (newPos >= pManager->totalFrames) {
            pManager->appState->shouldPlayNext = true;
            pManager->audioState->store(STOPPED);
            return;
        }

        ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);
        pManager->audioState->store(pManager->pausedWhileSeeking ? PAUSED : PLAYING);
        pManager->pausedWhileSeeking = false;

        return;
    }

    // If total frames were to be zero, then that means the file hasn't loaded yet
    if (currentState == SEEKING_BWD && pManager->totalFrames != 0) {
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame); // Clearing the buffer to avoid a weird glitch sound

        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(now - pManager->lastBackSeekTime);

        // Update timestamp immediately so the next callback can calculate the double-press window
        pManager->lastBackSeekTime = now;

        const auto DOUBLE_PRESS_WINDOW = std::chrono::milliseconds(500);

        // if we are in the first five seconds of the audio
        if (pManager->frameCursor <= pManager->frameOffset) {

            if (timeSinceLastClick < DOUBLE_PRESS_WINDOW) {
                // If double-pressed near start, signal a previous track change and stop current thread
                pManager->appState->shouldPlayPrev = true;
                pManager->audioState->store(STOPPED); // Signal loop to exit
                return;
            }
            else {
                // Single press near the start just resets the audio to 0
                pManager->frameCursor = 0;
                ma_decoder_seek_to_pcm_frame(&pManager->decoder, 0);
            }

        }
        else {
            // If further in the song, just perform a 5second seek back
            ma_uint64 newPos = pManager->frameCursor - pManager->frameOffset;
            ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);
        }

        // Return to the previous playback state to stop the seek loop
        pManager->audioState->store(pManager->pausedWhileSeeking ? PAUSED : PLAYING);
        pManager->pausedWhileSeeking = false;

        return;
    }


    if (pManager->audioState->load() == PLAYING) {
        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&pManager->decoder, pOutput, frameCount, &framesRead);

        // If the decoder provides fewer frames than requested, it means we hit the end of the file
        if (framesRead < frameCount) {
            pManager->audioFinished = true;
            pManager->audioState->store(STOPPED);
        }
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

    // Retrieve file length in frames and calculate total duration in seconds
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    totalSeconds = static_cast<double>(totalFrames) / deviceConfig.sampleRate;

    totalElapsedTime = 0;
    audioFinished = false;

    frameOffset = 5 * deviceConfig.sampleRate; // 5 seconds in frame

    pausedWhileSeeking = false;
    audioState->store(PLAYING);

    // Set initial seek time to the past to avoid triggering double-press on first load
    lastBackSeekTime = std::chrono::steady_clock::now() - std::chrono::seconds(1);

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

    auto lastTime = std::chrono::high_resolution_clock::now();


    while (audioState->load() != STOPPED) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Sync the current frame cursor with the decoder position
        ma_decoder_get_cursor_in_pcm_frames(&decoder, &frameCursor);

        totalElapsedTime = static_cast<double>(frameCursor) / deviceConfig.sampleRate;

        // Increment timer for wave movement
        visTimer += 6.0 * deltaTime;

        // Handling the visualizer by making a smooth fade in/fade out depending on state
        if (audioState->load() == PLAYING) {
            ma_device_start(&device);
            if (amplitude < 1.0) amplitude += 4.0 * deltaTime;
        }
        else if (audioState->load() == PAUSED) {
            // NOTE: The actual pause happens in data_callback()
            if (amplitude > 0) amplitude -= 4.0 * deltaTime;
        }

        // Update display state (but skip during resize to avoid flickering)
        if (!appState->shouldResize) {
            formatElapsed();
            displayState->totalSeconds = totalSeconds;
            displayState->totalElapsedTime = totalElapsedTime;
            displayState->amplitude = amplitude;
            displayState->visTimer = visTimer;
            displayState->shouldRedraw = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }

    // Clean up or trigger autoplay next track
    if (audioFinished && !appState->shouldPlayPrev) appState->shouldPlayNext = true;
    else if (!appState->shouldPlayNext && !appState->shouldPlayPrev) uninitializeAppState(appState, displayState);

    uninitializeMA();
}

void AudioManager::uninitializeMA() {
    ma_device_stop(&device);
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}

void AudioManager::playNext(std::atomic<AudioState>& audioState, AppState& appState) {
    // Increment index with wrapping to the start of the list
    appState.playingIndex = (appState.playingIndex + 1) % appState.numberOfFiles;

    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.playingIndex].c_str())};
    this->triggerAudioThread(&appState.audioDisplay, &appState, &audioState, audioFilePath);


    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
    appState.shouldRedraw = true;

    appState.shouldPlayNext = false;
}

void AudioManager::playPrevious(std::atomic<AudioState> &audioState, AppState &appState) {
    // Decrement index with wrapping to the end of the list
    appState.playingIndex = (appState.playingIndex - 1 + appState.numberOfFiles) % appState.numberOfFiles;

    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.playingIndex].c_str())};
    this->triggerAudioThread(&appState.audioDisplay, &appState, &audioState, audioFilePath);

    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
    appState.shouldRedraw = true;

    appState.shouldPlayPrev = false;
}