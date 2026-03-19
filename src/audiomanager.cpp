#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

#define MINIAUDIO_IMPLEMENTATION
#include "ncursesw/ncurses.h"
#include "audiomanager.h"
#include "states.h"
#include "tuisetup.h"

void uninitializePlayerState(PlayerState* playerState) {
    playerState->isPlaying = false;
    playerState->playingIndex = -1;
    playerState->audioName = "";
    playerState->duration = "";

    playerState->shouldRedraw = true;
}

std::string AudioManager::getAudioDuration() {
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss<< minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);

    std::string duration = ss.str();

    return duration;
}

void AudioManager::displayAudioInfo(WINDOW* audioInfoWindow, PlayerState* playerState) {
    playerState->duration = getAudioDuration();

    wmove(audioInfoWindow, 1, 2);
    wclrtoeol(audioInfoWindow);
    wprintw(audioInfoWindow, "Now Playing: %s", playerState->audioName.c_str());

    wmove(audioInfoWindow, 2, 2);
    wclrtoeol(audioInfoWindow);
    wprintw(audioInfoWindow, "Time: %d:%02d/%s",elapsedMinutes, static_cast<int>(elapsedSeconds), playerState->duration.c_str());

    wmove(audioInfoWindow, 3, 2);
    wclrtoeol(audioInfoWindow);
    wprintw(audioInfoWindow, "Status: %s", playerState->status.c_str());

    createBorder(&audioInfoWindow);
    wrefresh(audioInfoWindow);
}

void AudioManager::playAudio(WINDOW* audioInfoWindow, char* file, std::atomic<AudioState>* currState, PlayerState* playerState) {
    ma_engine_init(NULL, &engine);
    result = ma_sound_init_from_file(&engine, file, 0, NULL, NULL, &sound);

    ma_sound_get_length_in_seconds(&sound, &totalSeconds);

    if (result != MA_SUCCESS) {
        exit(-1);
    }

    ma_sound_start(&sound);

    currState->store(PLAYING);

    elapsedMinutes = 0;
    elapsedSeconds = 0;

    while (currState->load() != STOPPED && !ma_sound_at_end(&sound)) {
        if (currState->load() == PAUSED) {
            ma_sound_stop(&sound);
            playerState->status = "Paused";
        }
        else if (currState->load() == PLAYING) {
            ma_sound_start(&sound);
            elapsedSeconds += 0.5;
            playerState->status = "Playing";

            if (elapsedSeconds >= 60) {
                elapsedMinutes++;
                elapsedSeconds = 0;
            }
        }

        displayAudioInfo(audioInfoWindow, playerState);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    uninitializePlayerState(playerState);
    uninit();
}

void AudioManager::uninit() {
    totalSeconds = 0;
    remainingSeconds = 0;
    elapsedMinutes = 0;
    elapsedSeconds = 0;
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
}