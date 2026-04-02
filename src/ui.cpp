#include <locale.h>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>

#include <ncursesw/ncurses.h>

#include "states.hpp"
#include "ui.hpp"
#include "audiomanager.hpp"

void initializeTerminal() {
    setlocale(LC_ALL, "");
    initscr();
    start_color();


    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_WHITE, COLOR_RED);
    init_pair(4, COLOR_BLUE, 0);

    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
}

void initializeWindows(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow) {
    int terminalHeight;
    int terminalWidth;

    getmaxyx(stdscr, terminalHeight, terminalWidth);

    int leftWinWidth = terminalWidth/2;
    int rightWinWidth = terminalWidth - leftWinWidth;
    int midWinHeight = terminalHeight / 2;

    fileWindow = newwin(terminalHeight, leftWinWidth, 0, 0);
    audioInfoWindow = newwin(midWinHeight, rightWinWidth, 0, leftWinWidth);
    audioVisualWindow = newwin(terminalHeight - midWinHeight, rightWinWidth, midWinHeight, leftWinWidth);

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

void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow,
    std::atomic<AudioState>& audioState, AudioState& prevAudioState,
    AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastTime) {

    auto now = std::chrono::steady_clock::now();
    if (now - lastTime >= std::chrono::milliseconds(150)) {
        if (fileWindow) delwin(fileWindow);
        if (audioInfoWindow) delwin(audioInfoWindow);
        if (audioVisualWindow) delwin(audioVisualWindow);

        // Hard reset ncurses to recalculate internal terminal dimensions
        endwin();
        refresh();
        clear();

        initializeWindows(fileWindow, audioInfoWindow, audioVisualWindow);

        appState.shouldResize = false;
        appState.shouldRedraw = true;
        audioState.store(prevAudioState);
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
            else if (appState.isPlaying && currentItem == appState.playingIndex) pair = 2;

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
    appState.audioFiles = getAudioFiles();
    appState.numberOfFiles = appState.audioFiles.size();

    // Helps maintain playing highlighter after refresh
    if (appState.isPlaying) {
        for (int i = 0; i < appState.audioFiles.size(); i++) {
            if (appState.audioName == appState.audioFiles[i].filename()) {
                appState.playingIndex = i;
                break;
            }
        }
    }
    werase(fileWindow);

    appState.shouldRefreshFiles = false;
    appState.shouldRedraw = true;
}


//-----------------------------------------------------PRIVATE----------------------------------------------------------
namespace {
    void renderOscilloscope(WINDOW*& audioVisualInfo, AudioDisplayState& displayState) {
        int winHeight, winWidth;
        getmaxyx(audioVisualInfo, winHeight, winWidth);

        int centerY = winHeight / 2;
        double frequency = 0.1;
        int maxAmplitude = 4;

        wattron(audioVisualInfo, COLOR_PAIR(4));

        for (int x = 0; x < winWidth; x++) {
            double sineVal = std::sin((x * frequency) - displayState.visTimer);
            double harmonic = std::sin((x * frequency * 2.5) + (displayState.visTimer * 0.5)) * 0.3;
            int yOffset = static_cast<int>((sineVal + harmonic) * maxAmplitude * displayState.amplitude);
            int finalY = centerY + yOffset;

            if (finalY > 0 && finalY < winHeight - 1) mvwaddwstr(audioVisualInfo, finalY, x, L"━");
        }

        wattroff(audioVisualInfo, COLOR_PAIR(4));
    }

    void renderProgressBar(WINDOW*& audioInfoWindow, int windowWidth, AudioDisplayState& displayState) {
        int rightSidePadding = 5;
        int leftSidePadding = 2;
        int yPos = 3;
        int barWidth = windowWidth - rightSidePadding;
        double progress = displayState.totalSeconds > 0 ? displayState.totalElapsedTime / displayState.totalSeconds : 0;
        double filled = progress * barWidth;
        
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) mvwaddwstr(audioInfoWindow, yPos, i+leftSidePadding, L"█");
            else mvwaddwstr(audioInfoWindow, yPos, i+leftSidePadding, L"▒");
        }

    }
}
//----------------------------------------------------------------------------------------------------------------------


void displayAudioInfo(WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AudioDisplayState& displayState) {
    int xPadding = 2;

    werase(audioVisualWindow);

    renderOscilloscope(audioVisualWindow, displayState);

    wattron(audioInfoWindow, COLOR_PAIR(4));

    wmove(audioInfoWindow, 1, xPadding);
    wclrtoeol(audioInfoWindow);

    wattron(audioInfoWindow, WA_BOLD);
    wprintw(audioInfoWindow, "Now Playing: %s", displayState.audioName.c_str());

    int winWidth = getmaxx(audioInfoWindow);
    renderProgressBar(audioInfoWindow, winWidth, displayState);

    wmove(audioInfoWindow, 4, xPadding);
    wclrtoeol(audioInfoWindow);
    wprintw(audioInfoWindow, "Time: %d:%02d/%s",
            displayState.elapsedMinutes, displayState.elapsedSeconds, displayState.duration.c_str());

    wattroff(audioInfoWindow, COLOR_PAIR(4));
    wattroff(audioInfoWindow, WA_BOLD);

    // Recreating the border since adding things to the window removes some parts :P
    createBorder(audioInfoWindow);
    createBorder(audioVisualWindow);

    refresh();
    wrefresh(audioInfoWindow);
    wrefresh(audioVisualWindow);

    displayState.shouldRedraw = false;
}