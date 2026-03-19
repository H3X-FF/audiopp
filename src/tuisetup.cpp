#include <locale.h>
#include "tuisetup.h"
#include "ncursesw/ncurses.h"

void initializeTerminal() {
    initscr();

    start_color();

    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);

    setlocale(LC_ALL, "");

    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
}

void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow) {
    int terminalHeight, terminalWidth;
    getmaxyx(stdscr, terminalHeight, terminalWidth);

    int height{LINES-3};
    int width{COLS/2 - 1};

    *fileWindow = newwin(height, width, 0, 0);
    *audioInfoWindow = newwin(height, width, 0, width);
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
