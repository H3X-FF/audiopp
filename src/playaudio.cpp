#include <iostream>
#include <thread>
#include <chrono>

#define MINIAUDIO_IMPLEMENTATION
#include "playaudio.h"

void AudioPlayer::playAudio(char *file, std::atomic<AudioState>* currState, PlayerState* playerState) {
    ma_engine_init(NULL, &engine);
    result = ma_sound_init_from_file(&engine, file, 0, NULL, NULL, &sound);

    if (result != MA_SUCCESS) {
        exit(-1);
    }

    ma_sound_start(&sound);

    currState->store(PLAYING);

    while (currState->load() != STOPPED) {
        if (currState->load() == PAUSED) ma_sound_stop(&sound);
        else if (currState->load() == PLAYING) ma_sound_start(&sound);
        else std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    uninit();
}

void AudioPlayer::uninit() {
    ma_engine_uninit(&engine);
}
