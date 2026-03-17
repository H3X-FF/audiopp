#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>

#define MINIAUDIO_IMPLEMENTATION
#include "playaudio.h"

#include <ncurses/ncurses.h>

void uninitializePlayerState(PlayerState* playerState) {
    playerState->isPlaying = false;
    playerState->playingIndex = -1;
    playerState->audioName = "";
    playerState->duration = "";

    playerState->shouldRedraw = true;
}

void AudioPlayer::playAudio(char *file, std::atomic<AudioState>* currState, PlayerState* playerState) {
    ma_engine_init(NULL, &engine);
    result = ma_sound_init_from_file(&engine, file, 0, NULL, NULL, &sound);

    ma_sound_get_length_in_seconds(&sound, &totalSeconds);
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss<< minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);
    playerState->duration = ss.str();

    if (result != MA_SUCCESS) {
        exit(-1);
    }

    ma_sound_start(&sound);

    currState->store(PLAYING);

    elapsedMinutes = 0;
    elapsedSeconds = 0;

    while (currState->load() != STOPPED && !ma_sound_at_end(&sound)) {
        if (currState->load() == PAUSED) ma_sound_stop(&sound);
        else if (currState->load() == PLAYING) {
            ma_sound_start(&sound);
            elapsedSeconds += 0.5;

            if (elapsedSeconds >= 60) {
                elapsedMinutes++;
                elapsedSeconds = 0;
            }

            move(LINES-2, 0);
            printw("Time: %d:%02d/%s",elapsedMinutes, static_cast<int>(elapsedSeconds), playerState->duration.c_str());
        }
        clrtoeol();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    uninitializePlayerState(playerState);
    uninit();
}

void AudioPlayer::uninit() {
    totalSeconds = 0;
    remainingSeconds = 0;
    elapsedMinutes = 0;
    elapsedSeconds = 0;
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);
}