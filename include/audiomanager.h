#ifndef AUDIOPP_PLAYAUDIO_H
#define AUDIOPP_PLAYAUDIO_H

#include <atomic>

#include "miniaudio.h"
#include "states.h"
#include "ncursesw/ncurses.h"
class AudioManager {

    ma_engine engine;
    ma_sound sound;

    ma_result initializingSoundRes;
    ma_result gettingLengthRes;
    ma_result gettingFrameCursorsRes;

    ma_uint64 frameCursor;
    ma_uint32 sampleRate;

    float totalSeconds;
    float remainingSeconds;
    float totalElapsedTime;

    int elapsedMinutes;
    int elapsedSeconds;

    std::string progressBar;

    std::string getFullAudioDuration();
    void formatElapsed();

    void displayAudioInfo(WINDOW* audioInfoWindow, PlayerState* playerState);
    std::string renderProgressBar(WINDOW* audioInfoWindow);

public:
    // AudioPlayer();
    AudioState playAudio(WINDOW* audioInfoWindow, char* file, std::atomic<AudioState>* audioState, PlayerState* playerState);
    void uninit();
};

#endif //AUDIOPP_PLAYAUDIO_H