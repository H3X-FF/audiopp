#include "states.hpp"

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

#include <thread>
#include <chrono>

#include "environment.hpp"

void initializeAppState(AppState& appState) {
    appState.shouldRedraw = true;
    appState.shouldCheckForScroll = false;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;
    appState.inCommandMode = false;
    appState.shouldPlayNext = false;
    appState.shouldPlayPrev = false;

    appState.audioppPath = getAudioppPath();
    appState.audioppJsonFile = getAudioppJsonFile(appState.audioppPath);
    appState.audioppSettingsFile = getAudioppSettingsFile(appState.audioppPath);

    appState.topIndex = 0;
    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;

    appState.vfs.currPlaylist = "main";
}

void initializeAudioDisplayState(AudioDisplayState& audioDisplay) {
    audioDisplay.audioName = "";

    audioDisplay.elapsedMinutes = 0;
    audioDisplay.elapsedSeconds = 0;

    audioDisplay.totalSeconds = 0;
    audioDisplay.totalElapsedTime = 0;
    audioDisplay.amplitude = 0.0;
    audioDisplay.visTimer = 0.0;

    audioDisplay.shouldDrawAudioInfo = false;
    audioDisplay.shouldRenderAnimation = false;
    audioDisplay.shouldCleanup = false;
    audioDisplay.shouldUpdateVolOrRepeatTxt = false;
}

void printError(std::string msg) {
    move(LINES-1, 0);
    clrtoeol();
    wbkgdset(stdscr, COLOR_PAIR(3) | A_BOLD);
    printw("%s", msg.c_str());
    refresh();
    wbkgdset(stdscr, A_NORMAL);

    //std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    //move(LINES-1, 0);
    //clrtoeol();

    refresh();
}