#include <locale.h>

#include "states.hpp"
#include "tuimanager.hpp"
#include "ncursesw/ncurses.h"

void initializeTerminal() {
    initscr();
    start_color();

    // Color Pair Definitions:
    // 1: Selection Highlight
    // 2: Currently Playing Highlight
    // 3: Error/Alert Style
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_RED, COLOR_WHITE);

    setlocale(LC_ALL, ""); // Required for wide characters/UTF-8 borders

    curs_set(0);           // Hide physical terminal cursor
    keypad(stdscr, TRUE);  // Enable arrow keys
    noecho();              // Don't echo input to screen
    cbreak();              // Disable line buffering
    nodelay(stdscr, TRUE); // Non-blocking getch()
}

void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow) {
    int terminalHeight;
    int terminalWidth;

    getmaxyx(stdscr, terminalHeight, terminalWidth);

    int height{terminalHeight - 4};
    int width{terminalWidth/2 - 1};

    // Split screen vertically into two equal halves
    *fileWindow = newwin(height, width, 0, 0);
    *audioInfoWindow = newwin(height, width, 0, terminalWidth/2);
}

void createBorder(WINDOW** window) {
    cchar_t vline, hline, ulc, urc, llc, lrc;

    setcchar(&vline, L"║", A_NORMAL, 0, NULL);
    setcchar(&hline, L"═", A_NORMAL, 0, NULL);
    setcchar(&ulc,   L"╔", A_NORMAL, 0, NULL);
    setcchar(&urc,   L"╗", A_NORMAL, 0, NULL);
    setcchar(&llc,   L"╚", A_NORMAL, 0, NULL);
    setcchar(&lrc,   L"╝", A_NORMAL, 0, NULL);

    wborder_set(*window, &vline, &vline, &hline, &hline, &ulc, &urc, &llc, &lrc);
}