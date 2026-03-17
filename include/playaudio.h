#ifndef AUDIOPP_PLAYAUDIO_H
#define AUDIOPP_PLAYAUDIO_H

#include "miniaudio.h"
#include "states.h"

class AudioPlayer {

    ma_result result;
    ma_engine engine;
    ma_sound sound;

public:
    // AudioPlayer();
    void playAudio(char* file, std::atomic<AudioState>* currState, PlayerState* playerState);
    void uninit();
};

#endif //AUDIOPP_PLAYAUDIO_H