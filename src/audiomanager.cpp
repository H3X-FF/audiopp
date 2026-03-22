#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

#define MINIAUDIO_IMPLEMENTATION

#include "miniaudio/miniaudio.h"
#include "ncursesw/ncurses.h"
#include "audiomanager.h"
#include "states.h"
#include "tuimanager.h"

void uninitializeAppState(AppState* appState) {
    appState->isPlaying = false;
    appState->playingIndex = -1;
    appState->audioName = "";
    appState->duration = "";

    appState->shouldRedraw = true;
}

std::string AudioManager::getFullAudioDuration() {
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss<< minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);

    std::string duration = ss.str();

    return duration;
}

void AudioManager::formatElapsed() {
    elapsedMinutes = static_cast<int>(totalElapsedTime) / 60;
    elapsedSeconds = static_cast<int>(totalElapsedTime) % 60;
}

void AudioManager::displayAudioInfo(WINDOW** audioInfoWindow, AppState* appState) {
    appState->duration = getFullAudioDuration();

    werase(*audioInfoWindow);

    wmove(*audioInfoWindow, 1, 2);
    // wclrtoeol(audioInfoWindow);
    wprintw(*audioInfoWindow, "Now Playing: %s", appState->audioName.c_str());

    wmove(*audioInfoWindow, 2, 2);
    // wclrtoeol(audioInfoWindow);
    wprintw(*audioInfoWindow, "Status: %s", appState->status.c_str());


    formatElapsed();

    wmove(*audioInfoWindow, 3, 2);
    // wclrtoeol(audioInfoWindow);
    wprintw(*audioInfoWindow, "Time: %d:%02d/%s", elapsedMinutes, elapsedSeconds, appState->duration.c_str());

    wmove(*audioInfoWindow, 4, 2);
    wprintw(*audioInfoWindow, "%s", progressBar.c_str());

    createBorder(audioInfoWindow);
    wrefresh(*audioInfoWindow);
}

std::string AudioManager::renderProgressBar(WINDOW** audioInfoWindow) {
    int padding{10};
    int barWidth{getmaxx(*audioInfoWindow) - padding};
    double progress{totalElapsedTime / totalSeconds};
    double filled{progress * barWidth};

    std::string bar{"["};

    for (int i{0}; i < barWidth; i++) {

        if (i < filled) bar += '#';
        else bar += '-';
    }

    bar += ']';

    return bar;
}

AudioState AudioManager::playAudio(WINDOW** audioInfoWindow, char* file, std::atomic<AudioState>* currState, AppState* appState) {
    ma_engine_init(NULL, &engine);

    initializingSoundRes = ma_sound_init_from_file(&engine, file, 0, NULL, NULL, &sound);
    gettingLengthRes = ma_sound_get_length_in_seconds(&sound, &totalSeconds);

    if (initializingSoundRes != MA_SUCCESS || gettingLengthRes != MA_SUCCESS) return FAILED;

    ma_sound_get_data_format(&sound, NULL, NULL, &sampleRate, NULL, 0);

    ma_sound_start(&sound);

    currState->store(PLAYING);

    totalElapsedTime = 0;

    while (currState->load() != STOPPED && !ma_sound_at_end(&sound)) {

        ma_sound_get_cursor_in_pcm_frames(&sound, &frameCursor);
        totalElapsedTime = static_cast<double>(frameCursor) / sampleRate;

        if (currState->load() == PAUSED) {
            ma_sound_stop(&sound);

            appState->status = "Paused";
        }
        else if (currState->load() == PLAYING) {
            ma_sound_start(&sound);

            appState->status = "Playing";
        }

        if (currState->load() != RESIZING) {
            progressBar = renderProgressBar(audioInfoWindow);
            displayAudioInfo(audioInfoWindow, appState);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Temporary until I add a play next feature :)
    if (ma_sound_at_end(&sound)) {
        wmove(*audioInfoWindow, 2, 2);
        wclrtoeol(*audioInfoWindow);
        wprintw(*audioInfoWindow, "Status: Finished");
        wrefresh(*audioInfoWindow);
        refresh();
    }
    //--------------------------------------------------

    uninitializeAppState(appState);
    uninit();

    return SUCCESS;
}

void AudioManager::uninit() {
    totalSeconds = 0;
    remainingSeconds = 0;
    totalElapsedTime = 0;

    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
}