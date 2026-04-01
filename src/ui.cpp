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
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_WHITE, COLOR_RED);
    init_pair(4, COLOR_GREEN, 0);

    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
}

void initializeWindows(WINDOW*& fileWindow, WINDOW*& audioInfoWindow) {
    int terminalHeight;
    int terminalWidth;

    getmaxyx(stdscr, terminalHeight, terminalWidth);

    int height{terminalHeight - 4};
    int width{terminalWidth/2 - 1};

    // Split screen vertically into two equal halves
    fileWindow = newwin(height, width, 0, 0);
    audioInfoWindow = newwin(height, width, 0, terminalWidth/2);
}

void createBorder(WINDOW*& window) {
    cchar_t vline, hline, ulc, urc, llc, lrc;

    setcchar(&vline, L"║", WA_NORMAL, 0, NULL);
    setcchar(&hline, L"═", WA_NORMAL, 0, NULL);
    setcchar(&ulc,   L"╔", WA_NORMAL, 0, NULL);
    setcchar(&urc,   L"╗", WA_NORMAL, 0, NULL);
    setcchar(&llc,   L"╚", WA_NORMAL, 0, NULL);
    setcchar(&lrc,   L"╝", WA_NORMAL, 0, NULL);

    wborder_set(window, &vline, &vline, &hline, &hline, &ulc, &urc, &llc, &lrc);
}

void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow,
    std::atomic<AudioState>& audioState, AudioState& prevAudioState,
    AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastTime) {

    auto now{std::chrono::steady_clock::now()};
    if (now - lastTime >= std::chrono::milliseconds(150)) {
        if (fileWindow) delwin(fileWindow);
        if (audioInfoWindow) delwin(audioInfoWindow);

        // Hard reset ncurses to recalculate internal terminal dimensions
        endwin();
        refresh();
        clear();

        initializeWindows(fileWindow, audioInfoWindow);

        appState.shouldResize = false;
        appState.shouldRedraw = true;
        audioState.store(prevAudioState);
    }

}

void displayFiles(WINDOW*& fileWindow, AppState& appState) {
    int pair;

    for (int i{0}; i < appState.audioFiles.size(); i++) {
        pair = 0;

        if (i == appState.currSelectionIndex) pair = 1;
        else if (appState.isPlaying && i == appState.playingIndex) pair = 2;

        wattron(fileWindow, COLOR_PAIR(pair));
        mvwprintw(fileWindow, i+1, 2, "%d. %s\n", i+1, appState.audioFiles[i].filename().c_str());
        wattroff(fileWindow, COLOR_PAIR(pair));
    }
}

void redrawScreen(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, AppState& appState) {
    displayFiles(fileWindow, appState);

    createBorder(fileWindow);
    createBorder(audioInfoWindow);

    refresh();
    wrefresh(fileWindow);
    wrefresh(audioInfoWindow);

    appState.shouldRedraw = false;
}

void refreshFiles(WINDOW*& fileWindow, AppState& appState) {
    appState.audioFiles = getAudioFiles();
    appState.numberOfFiles = appState.audioFiles.size();

    // Helps maintain playing highlighter after refresh
    if (appState.isPlaying) {
        for (int i{0}; i < appState.audioFiles.size(); i++) {
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
    void renderOscilloscope(WINDOW*& audioInfoWindow, double amplitude, double visTimer) {
        int winHeight, winWidth;
        getmaxyx(audioInfoWindow, winHeight, winWidth);

        int centerY = winHeight / 2;
        double frequency = 0.1;
        int maxAmplitude = 4;

        wattron(audioInfoWindow, COLOR_PAIR(4));

        for (int x = 0; x < winWidth; x++) {
            double sineVal = std::sin((x * frequency) - visTimer);
            double harmonic = std::sin((x * frequency * 2.5) + (visTimer * 0.5)) * 0.3;
            int yOffset = static_cast<int>((sineVal + harmonic) * maxAmplitude * amplitude);
            int finalY = centerY + yOffset;

            if (finalY > 0 && finalY < winHeight - 1) mvwaddwstr(audioInfoWindow, finalY, x, L"━");
        }

        wattroff(audioInfoWindow, COLOR_PAIR(4));
    }

    std::string renderProgressBar(int windowWidth, int totalSeconds, double totalElapsedTime) {
        int padding = 10;
        int barWidth = windowWidth - padding;
        double progress = totalSeconds > 0 ? totalElapsedTime / totalSeconds : 0;
        double filled = progress * barWidth;

        std::string bar = "[";
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) bar += '#';
            else bar += '-';
        }
        bar += ']';

        return bar;
    }
}
//----------------------------------------------------------------------------------------------------------------------


void displayAudioInfo(WINDOW*& audioInfoWindow, AudioDisplayState& displayState) {
    int xPadding = 2;

    werase(audioInfoWindow);

    renderOscilloscope(audioInfoWindow, displayState.amplitude, displayState.visTimer);

    int winWidth = getmaxx(audioInfoWindow);
    std::string progressBar = renderProgressBar(winWidth, displayState.totalSeconds, displayState.totalElapsedTime);

    // Draw text info
    wmove(audioInfoWindow, 1, xPadding);
    wprintw(audioInfoWindow, "Now Playing: %s", displayState.audioName.c_str());

    wmove(audioInfoWindow, 2, xPadding);
    wprintw(audioInfoWindow, "Time: %d:%02d/%s",
            displayState.elapsedMinutes, displayState.elapsedSeconds, displayState.duration.c_str());

    wmove(audioInfoWindow, 3, xPadding);
    wprintw(audioInfoWindow, "%s", progressBar.c_str());

    createBorder(audioInfoWindow);

    wrefresh(audioInfoWindow);

    displayState.shouldRedraw = false;
}