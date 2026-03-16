#include <iostream>
#include <thread>
#include <chrono>

#define MINIAUDIO_IMPLEMENTATION
#include "playaudio.h"

void AudioPlayer::playAudio(char *file, std::atomic<AudioState>* currState) {

    result = ma_engine_init(NULL, &engine);

    if (result != MA_SUCCESS) {
        std::cout << "Failed to play audio!\n";
        exit(-1);
    }

    ma_engine_play_sound(&engine, file, NULL);
    currState->store(PLAYING);


    while (currState->load() == PLAYING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    uninit();

}

void AudioPlayer::uninit() {
    ma_engine_uninit(&engine);
}
