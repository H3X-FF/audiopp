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

void AudioManager::triggerAudioThread(WINDOW** infoWin, AppState* aState, std::atomic<AudioState>* audioAtomic, char* filePath) {
    audioInfoWindow = infoWin;
    appState = aState;
    audioState = audioAtomic;
    audioFile = filePath;

    audioState->store(STOPPED);

    if (audioThread.joinable()) audioThread.join();

    audioThread = std::thread(&AudioManager::playAndManageAudio, this);
}

AudioState AudioManager::initializeMA() {
    ma_engine_init(NULL, &engine);

    initializingSoundRes = ma_sound_init_from_file(&engine, audioFile, 0, NULL, NULL, &sound);
    gettingLengthRes = ma_sound_get_length_in_seconds(&sound, &totalSeconds);

    if (initializingSoundRes != MA_SUCCESS || gettingLengthRes != MA_SUCCESS) return FAILED;

    ma_sound_get_data_format(&sound, NULL, NULL, &sampleRate, NULL, 0);

    totalFrames = totalSeconds * sampleRate;
    frameOffset = 5 * sampleRate; // 5 seconds in frame

    ma_sound_start(&sound);

    audioState->store(PLAYING);
    duration = getFullAudioDuration();

    return SUCCESS;
}

void AudioManager::playAndManageAudio() {
    if (initializeMA() == FAILED) return;

    auto lastTime{std::chrono::high_resolution_clock::now()};

    // The audio thread settles in here. It performs action depending on the audio state
    while (audioState->load() != STOPPED && !ma_sound_at_end(&sound)) {

        auto currentTime{std::chrono::high_resolution_clock::now()};
        double dt{std::chrono::duration<double>(currentTime - lastTime).count()};
        lastTime = currentTime;

        ma_sound_get_cursor_in_pcm_frames(&sound, &frameCursor);
        totalElapsedTime = static_cast<double>(frameCursor) / sampleRate;

        // Increment timer for wave movement
        visTimer += 5.0 * dt;

        // Handling the visualizer by making a smooth fade in/fade out depending on state
        if (audioState->load() == PLAYING) {
            ma_sound_start(&sound);
            status = "Playing";
            if (amplitude < 1.0) amplitude += 4.0 * dt;
        }
        else if (audioState->load() == PAUSED) {
            ma_sound_stop(&sound);
            status = "Paused";
            if (amplitude > 0) amplitude -= 4.0 * dt;
        }

        if (audioState->load() == SEEKING_FWD) {
            ma_uint64 newPos{frameCursor + frameOffset};

            /*
             * ---------------------------------------------------------------------------------------------------------
             * Note here: no need to check if the new seeked position is greater than the total frames within the audio.
             * It will already be considered at the end of the file, so by default playNext will be true.
             * ---------------------------------------------------------------------------------------------------------
             */


            ma_sound_seek_to_pcm_frame(&sound, newPos);
            audioState->store(PLAYING);
        }
        else if (audioState->load() == SEEKING_BWD) {
            ma_uint64 newPos = (frameCursor > frameOffset) ? (frameCursor - frameOffset) : 0;

            /* while miniaudio will consider 'frameCursor < 0' at end of file
             * due to the frameCursor having the value of a 64-bit unsigned integer, this is placed here so we can play
             * the previous track instead of the default next track */
            if (newPos == 0) appState->playPrev = true;

            ma_sound_seek_to_pcm_frame(&sound, newPos);
            audioState->store(PLAYING);
        }

        if (audioState->load() != RESIZING) {
            progressBar = renderProgressBar(audioInfoWindow);
            displayAudioInfo(audioInfoWindow, appState);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    // The file being at the end and playPrev being false means that the user didn't seek backwards all the way (to the start of the track)
    if (ma_sound_at_end(&sound) && !appState->playPrev) appState->playNext = true;
    /* If neither are true, then that means the playing audio file hasn't reached the end,
     * so we uninitialize the app state */
    else if (!appState->playNext && !appState->playPrev) uninitializeAppState(appState);

    uninitializeMA();
}

void AudioManager::uninitializeMA() {
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
}