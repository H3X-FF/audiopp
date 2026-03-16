#ifndef AUDIOPP_PLAYAUDIO_H
#define AUDIOPP_PLAYAUDIO_H

#include "miniaudio.h"
#include "states.h"

class AudioPlayer {

    ma_engine engine;
    ma_result result;

public:
    void playAudio(char* file, std::atomic<AudioState>* currState);
    void uninit();
};

#endif //AUDIOPP_PLAYAUDIO_H