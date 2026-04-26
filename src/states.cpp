#include "states.hpp"
#include <algorithm>
#include <ncursesw/ncurses.h>
#include <thread>
#include <chrono>

#include"sort_commands.hpp"

void initializeVfs(VirtualFS& vfs) {
    vfs.currPlaylist = "main";
}

void initializeAppState(AppState& appState) {
    appState.shouldRedraw = true;
    appState.shouldCheckForScroll = false;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;
    appState.inCommandMode = false;
    appState.shouldPlayNext = false;
    appState.shouldPlayPrev = false;

    appState.topIndex = 0;
    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;
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

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    move(LINES-1, 0);
    wbkgdset(stdscr, A_NORMAL);
    clrtoeol();

    refresh();
}