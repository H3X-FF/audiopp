#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

#define MINIAUDIO_IMPLEMENTATION
#include "playaudio.h"

#include <ncurses/ncurses.h>

void uninitializePlayerState(PlayerState* playerState) {
    playerState->isPlaying = false;
    playerState->playingIndex = -1;
    playerState->shouldRedraw = true;
    playerState->duration = "";
}

void AudioPlayer::playAudio(char *file, std::atomic<AudioState>* currState, PlayerState* playerState) {
    ma_engine_init(NULL, &engine);
    result = ma_sound_init_from_file(&engine, file, 0, NULL, NULL, &sound);

    ma_sound_get_length_in_seconds(&sound, &totalSeconds);
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);
    playerState->duration = std::to_string(minutes) + ":" + std::to_string(static_cast<int>(remainingSeconds));

    if (result != MA_SUCCESS) {
        exit(-1);
    }

    ma_sound_start(&sound);

    currState->store(PLAYING);

    elapsedTime = 0;

    while (currState->load() != STOPPED && !ma_sound_at_end(&sound)) {
        if (currState->load() == PAUSED) ma_sound_stop(&sound);
        else if (currState->load() == PLAYING) ma_sound_start(&sound);

        move(LINES-2, 0);
        elapsedTime += 0.5;
        printw("Time: %f/%s", elapsedTime, playerState->duration.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    uninitializePlayerState(playerState);
    uninit();
}

void AudioPlayer::uninit() {
    ma_engine_uninit(&engine);}