#include "states.hpp"
#include <thread>

#include "environment.hpp"


#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

void initializeAppState(AppState& appState) {
    appState.shouldRedrawScreen = true;
    appState.shouldRedrawFileList = false;
    appState.shouldCheckForScroll = false;
    appState.shouldRefreshFiles = false;
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
    appState.errorTick = 0;

    appState.vfs.currPlaylist = "main";
}

void initializeAudioDisplayState(AudioInfoState& audioDisplay) {
    audioDisplay.audioName = "";

    audioDisplay.elapsedMinutes = 0;
    audioDisplay.elapsedSeconds = 0;

    audioDisplay.totalSeconds = 0;
    audioDisplay.totalElapsedTime = 0;;

    audioDisplay.shouldDrawAudioInfo = false;
    audioDisplay.shouldRenderAnimation = false;
    audioDisplay.shouldCleanup = false;
    audioDisplay.shouldUpdateVolOrRepeatTxt = false;
}

void printError(std::string msg, AppState& appState) {
    move(LINES-1, 0);
    clrtoeol();
    wbkgdset(stdscr, COLOR_PAIR(3) | A_BOLD);
    printw("%s", msg.c_str());
    refresh();
    wbkgdset(stdscr, A_NORMAL);

    appState.errorTick = 80;

    refresh();
}