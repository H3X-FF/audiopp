#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "ncursesw/ncurses.h"
#include "audiomanager.hpp"
#include "states.hpp"
#include "tuimanager.hpp"

void AudioManager::terminateAudioThread() {
    audioState->store(STOPPED);
    if (audioThread.joinable()) audioThread.join();
}

AudioManager::AudioManager() {
    totalElapsedTime = 0;

    maxAmplitude = 6;
    amplitude = 0.0;
    visTimer = 0.0;
}

// Resets UI-related playback state when audio stops.
void uninitializeAppState(AppState* appState) {
    appState->isPlaying = false;
    appState->playingIndex = -1;
    appState->audioName = "";
    appState->shouldRedraw = true;
}

std::string AudioManager::getFullAudioDuration() {
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss<< minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);
    return ss.str();
}

void AudioManager::formatElapsed() {
    elapsedMinutes = static_cast<int>(totalElapsedTime) / 60;
    elapsedSeconds = static_cast<int>(totalElapsedTime) % 60;
}

std::string AudioManager::renderProgressBar(WINDOW** audioInfoWindow) {
    int padding{10};
    int barWidth{getmaxx(*audioInfoWindow) - padding};
    double progress{totalSeconds > 0 ? totalElapsedTime / totalSeconds : 0};
    double filled{progress * barWidth};

    std::string bar{"["};
    for (int i{0}; i < barWidth; i++) {
        if (i < filled) bar += '#';
        else bar += '-';
    }
    bar += ']';

    return bar;
}

void AudioManager::renderOscilloscope(WINDOW** audioInfoWindow) {
    int winHeight, winWidth;
    getmaxyx(*audioInfoWindow, winHeight, winWidth);

    int centerY = winHeight / 2;
    double frequency = 0.1;

    wattron(*audioInfoWindow, COLOR_PAIR(4));

    for (int x = 0; x < winWidth; x++) {
        double sineVal = std::sin((x * frequency) - visTimer);

        double harmonic = std::sin((x * frequency * 2.5) + (visTimer * 0.5)) * 0.3;

        int yOffset = static_cast<int>((sineVal + harmonic) * maxAmplitude * amplitude);
        int finalY = centerY + yOffset;

        if (finalY > 0 && finalY < winHeight - 1) mvwaddwstr(*audioInfoWindow, finalY, x, L"━");

    }

    wattroff(*audioInfoWindow, COLOR_PAIR(4));
}

void AudioManager::displayAudioInfo(WINDOW** audioInfoWindow, AppState* appState) {
    int xPadding{2};

    werase(*audioInfoWindow);

    renderOscilloscope(audioInfoWindow);

    wmove(*audioInfoWindow, 1, xPadding);
    wprintw(*audioInfoWindow, "Now Playing: %s", appState->audioName.c_str());

    formatElapsed();

    wmove(*audioInfoWindow, 2, xPadding);
    wprintw(*audioInfoWindow, "Time: %d:%02d/%s", elapsedMinutes, elapsedSeconds, duration.c_str());

    wmove(*audioInfoWindow, 3, xPadding);
    wprintw(*audioInfoWindow, "%s", progressBar.c_str());

    createBorder(audioInfoWindow);

    wrefresh(*audioInfoWindow);
}

/*
* Plays audio and sets up the audio thread. It signals a stop first, checks if we can join the thread
* so that if there's an active thread that thread exits the loop, resets states
* then the new thread comes in and plays the new audio. */
void AudioManager::triggerAudioThread(WINDOW** infoWin, AppState* aState, std::atomic<AudioState>* audioAtomic, char* filePath) {
    audioInfoWindow = infoWin;
    appState = aState;
    audioState = audioAtomic;
    audioFile = filePath;

    audioState->store(STOPPED);

    if (audioThread.joinable()) audioThread.join();

    audioThread = std::thread(&AudioManager::playAndManageAudio, this);
}

/* A function used by miniaudio for delivering real-time PCM data. */
void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {

    AudioManager* pManager{static_cast<AudioManager*>(pDevice->pUserData)};

    if (pManager->audioState->load() == SEEKING_FWD) {

        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, bytesPerFrame); // Wipes the current buffer to prevent a pop sound.

        ma_uint64 newPos{pManager->frameCursor + pManager->frameOffset};

        if (newPos >= pManager->totalFrames) pManager->appState->shouldPlayNext = true;


        ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);

        pManager->audioState->store(PLAYING);

    }

    if (pManager->audioState->load() == SEEKING_BWD) {

        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, bytesPerFrame); //Wipes the current buffer to prevent a pop sound.

        ma_uint64 newPos{pManager->frameCursor - pManager->frameOffset};

        if (newPos <= 0) pManager->appState->shouldPlayPrev = true;


        ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);

        pManager->audioState->store(PLAYING);
    }

    ma_decoder_read_pcm_frames(&pManager->decoder, pOutput, frameCount, NULL);
}

// Initializes miniaudio. After initializing miniaudio, it will set the state to playing
AudioState AudioManager::initializeMA() {

    decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    decoderConfig.seekPointCount = 128;

    decoderInitRes = ma_decoder_init_file(audioFile, &decoderConfig, &decoder);

    if (decoderInitRes != MA_SUCCESS) {
        audioState->store(STOPPED);
        return FAILED;
    }

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

    frameOffset = 5 * deviceConfig.sampleRate; // 5 seconds in frame

    audioState->store(PLAYING);
    appState->isPlaying = true;
    duration = getFullAudioDuration();

    return SUCCESS;
}

void AudioManager::playAndManageAudio() {
    if (initializeMA() == FAILED) return;

    frameCursor = 0;

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
            status = "Playing";
            if (amplitude < 1.0) amplitude += 4.0 * dt;
        }
        else if (audioState->load() == PAUSED) {
            ma_device_stop(&device);
            status = "Paused";
            if (amplitude > 0) amplitude -= 4.0 * dt;
        }
        

        if (audioState->load() != RESIZING) {
            progressBar = renderProgressBar(audioInfoWindow);
            displayAudioInfo(audioInfoWindow, appState);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    // The file being at the end and playPrev being false means that the user didn't seek backwards all the way (to the start of the track)
    if (frameCursor >= totalFrames && !appState->shouldPlayPrev) appState->shouldPlayNext = true;
    /* If neither are true, then that means the playing audio file hasn't reached the end,
     * so we uninitialize the app state */
    else if (!appState->shouldPlayNext && !appState->shouldPlayPrev) uninitializeAppState(appState);

    uninitializeMA();
}

void AudioManager::uninitializeMA() {
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}