#include <locale.h>
#include <vector>
#include <string>
#include <chrono>

#include <ncursesw/ncurses.h>

#include "states.hpp"
#include "ui.hpp"
#include "animations.hpp"

void initializeTerminal() {
    setlocale(LC_ALL, "");
    initscr();
    start_color();


    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_WHITE, COLOR_RED);

    init_pair(4, COLOR_BLUE, 0);
    init_pair(5, COLOR_MAGENTA, 0);

    nl();
    curs_set(0);
    set_escdelay(25);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    timeout(60);
}

void initializeWindows(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow) {
    int terminalHeight;
    int terminalWidth;

    getmaxyx(stdscr, terminalHeight, terminalWidth);

    int winHeight = terminalHeight - 1;
    int midWinHeight = winHeight / 2;
    int fileWinWidth = terminalWidth/2;
    int rightWindowsWidth = terminalWidth - fileWinWidth;
    int audioVisualWinHeight = winHeight - midWinHeight;

    int& audioVisualY = midWinHeight;
    int& rightWindowsX = fileWinWidth;

    fileWindow = newwin(winHeight, fileWinWidth, 0, 0);
    audioInfoWindow = newwin(midWinHeight, rightWindowsWidth, 0, rightWindowsX);
    audioVisualWindow = newwin(audioVisualWinHeight, rightWindowsWidth, audioVisualY, rightWindowsX);

    scrollok(fileWindow, FALSE);
}

void createBorder(WINDOW*& window) {
    cchar_t vline, hline, ulc, urc, llc, lrc;

    setcchar(&vline, L"│", WA_NORMAL, 0, NULL);
    setcchar(&hline, L"─", WA_NORMAL, 0, NULL);
    setcchar(&ulc,   L"╭", WA_NORMAL, 0, NULL);
    setcchar(&urc,   L"╮", WA_NORMAL, 0, NULL);
    setcchar(&llc,   L"╰", WA_NORMAL, 0, NULL);
    setcchar(&lrc,   L"╯", WA_NORMAL, 0, NULL);

    wborder_set(window, &vline, &vline, &hline, &hline, &ulc, &urc, &llc, &lrc);
}

void scrollList(WINDOW*& fileWindow, AppState &appState) {
    int fileWindowHeight = getmaxy(fileWindow) - 2;

    if (appState.numberOfFiles <= fileWindowHeight) {
        appState.topIndex = 0;
    }
     else if (appState.currSelectionIndex < appState.topIndex) {
        appState.topIndex = appState.currSelectionIndex;
    }
     else if (appState.currSelectionIndex >= appState.topIndex + fileWindowHeight) {
        appState.topIndex = appState.currSelectionIndex - fileWindowHeight + 1;
    }


    if (appState.topIndex + fileWindowHeight > appState.numberOfFiles) {
        appState.topIndex = appState.numberOfFiles - fileWindowHeight;

        if (appState.topIndex < 0) appState.topIndex = 0;
    }
}

void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow,
               AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastTime) {

    auto now = std::chrono::steady_clock::now();
    if (now - lastTime >= std::chrono::milliseconds(150)) {

        werase(audioInfoWindow); // This for clearing the static text to avoid an ugly glitchy look during resize

        if (fileWindow) delwin(fileWindow);
        if (audioInfoWindow) delwin(audioInfoWindow);
        if (audioVisualWindow) delwin(audioVisualWindow);

        // Hard reset ncurses to recalculate internal terminal dimensions
        endwin();
        refresh();
        clear();

        initializeWindows(fileWindow, audioInfoWindow, audioVisualWindow);

        appState.shouldCheckForScroll = true;
        appState.shouldResize = false;
        appState.shouldRedraw = true;

        // Prevents drawing "info" after resizing but nothing is playing
        if (appState.playingIndex != -1) {
            appState.audioDisplayState.shouldDrawAudioInfo = appState.playingIndex != -1;
            appState.audioDisplayState.displayCurrRepeatMode = true;
        }
    }

}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

void displayFiles(WINDOW*& fileWindow, AppState& appState) {
    int pair;
    int fileWindowHeight = getmaxy(fileWindow) - 2;

    for (int i{0}; i < fileWindowHeight; i++) {
        int currentItem = appState.topIndex + i;
        pair = 0;

        wmove(fileWindow, i+1, 1);

        if (currentItem < appState.numberOfFiles) {
            if (currentItem == appState.currSelectionIndex) pair = 1;
            else if (currentItem == appState.playingIndex) pair = 2;

            wbkgdset(fileWindow, COLOR_PAIR(pair));
            wclrtoeol(fileWindow);
            mvwprintw(fileWindow, i+1, 2, "%d. %s", currentItem+1, appState.audioFiles[currentItem].filename().c_str());
            wbkgdset(fileWindow, A_NORMAL);
        }
        else {
            wbkgdset(fileWindow, A_NORMAL);
            wclrtoeol(fileWindow);
        }
    }

    wrefresh(fileWindow);
}

void redrawScreen(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AppState& appState) {
    displayFiles(fileWindow, appState);

    createBorder(fileWindow);
    createBorder(audioInfoWindow);
    createBorder(audioVisualWindow);

    refresh();
    wrefresh(fileWindow);
    wrefresh(audioInfoWindow);
    wrefresh(audioVisualWindow);

    appState.shouldRedraw = false;
}

void refreshFiles(WINDOW*& fileWindow, AppState& appState) {
    appState.audioFiles.clear();
    getAudioFiles(appState);
    appState.numberOfFiles = appState.audioFiles.size();

    // Helps maintain playing highlighter after refresh
    if (appState.playingIndex != -1) {
        for (int i = 0; i < appState.audioFiles.size(); i++) {
            if (appState.audioDisplayState.audioName == appState.audioFiles[i].filename()) {
                appState.playingIndex = i;
                break;
            }
        }
    }
    werase(fileWindow);

    appState.shouldRefreshFiles = false;
    appState.shouldRedraw = true;
}

void renderAnimations(WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AudioDisplayState& displayState) {
    werase(audioVisualWindow);

    renderOscilloscope(audioVisualWindow, displayState);

    int winWidth = getmaxx(audioInfoWindow);
    renderProgress(audioInfoWindow, winWidth, displayState);

    createBorder(audioInfoWindow);
    createBorder(audioVisualWindow);

    wnoutrefresh(audioInfoWindow);
    wnoutrefresh(audioVisualWindow);

    doupdate();

    displayState.shouldRenderAnimation = false;
}

const int X_POS = 2;

void displayAudioInfo(WINDOW*& audioInfoWindow, AudioDisplayState& displayState) {

    int yPos = 1;

    wattron(audioInfoWindow, COLOR_PAIR(4));
    wattron(audioInfoWindow, WA_BOLD);

    wmove(audioInfoWindow, yPos, X_POS);
    wclrtoeol(audioInfoWindow);

    wprintw(audioInfoWindow, "Now Playing: %s", displayState.audioName.c_str());

    wattroff(audioInfoWindow, COLOR_PAIR(4));
    wattroff(audioInfoWindow, WA_BOLD);


    // Recreating the border since adding things to the window removes some parts :P
    createBorder(audioInfoWindow);

    refresh();
    wrefresh(audioInfoWindow);


    displayState.shouldDrawAudioInfo = false;
}

void displayRepeatMode(WINDOW*& audioInfoWindow, AudioDisplayState& displayState) {
    int yPos = 6; // Note: timer is at y-pos 4

    wmove(audioInfoWindow, yPos, X_POS);
    wclrtoeol(audioInfoWindow);

    wattron(audioInfoWindow, COLOR_PAIR(4));
    wattron(audioInfoWindow, WA_BOLD);

    wprintw(audioInfoWindow, "Repeat Mode: %s", displayState.repeatModeStr.c_str());

    wattroff(audioInfoWindow, COLOR_PAIR(4));
    wattroff(audioInfoWindow, WA_BOLD);

    createBorder(audioInfoWindow);

    refresh();
    wrefresh(audioInfoWindow);

    displayState.displayCurrRepeatMode = false;
}

void cleanupAudioWindows(WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AudioDisplayState& displayState) {
    werase(audioInfoWindow);
    werase(audioVisualWindow);

    createBorder(audioInfoWindow);
    createBorder(audioVisualWindow);

    refresh();
    wrefresh(audioInfoWindow);
    wrefresh(audioVisualWindow);

    displayState.shouldCleanup = false;
}