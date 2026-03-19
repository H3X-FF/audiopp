#ifndef AUDIOPP_PLAYAUDIO_H
#define AUDIOPP_PLAYAUDIO_H

#include <atomic>

#include "miniaudio.h"
#include "states.h"
#include "ncursesw/ncurses.h"
class AudioManager {

    ma_result result;
    ma_engine engine;
    ma_sound sound;
    float totalSeconds;
    float remainingSeconds;
    int elapsedMinutes;
    float elapsedSeconds;

    std::string getAudioDuration();

    void displayAudioInfo(WINDOW* audioInfoWindow, PlayerState* playerState);

public:
    // AudioPlayer();
    void playAudio(WINDOW* audioInfoWindow, char* file, std::atomic<AudioState>* currState, PlayerState* playerState);
    void uninit();
};

#endif //AUDIOPP_PLAYAUDIO_H