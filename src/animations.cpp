#include <cmath>
#include <ncursesw/ncurses.h>

#include "animations.hpp"
#include "states.hpp"


void renderOscilloscope(WINDOW*& audioVisualInfo, AudioDisplayState& displayState) {
    int winHeight, winWidth;
    getmaxyx(audioVisualInfo, winHeight, winWidth);

    int centerY = winHeight / 2;
    double frequency = 0.1;
    int maxAmplitude = 3;

    wattron(audioVisualInfo, COLOR_PAIR(4));

    for (int x = 0; x < winWidth; x++) {
        double sineVal = displayState.amplitude * std::sin((x * frequency) - displayState.visTimer);
        double harmonic = displayState.amplitude * std::sin((x * frequency * 2.5) + (displayState.visTimer * 0.5)) * 0.3;
        int yOffset = static_cast<int>((sineVal + harmonic) * maxAmplitude * displayState.amplitude);
        int finalY = centerY + yOffset;

        if (finalY > 0 && finalY < winHeight - 1) mvwaddwstr(audioVisualInfo, finalY, x, L"━");
    }

    wattroff(audioVisualInfo, COLOR_PAIR(4));
}

void renderProgress(WINDOW*& audioInfoWindow, int windowWidth, AudioDisplayState& displayState) {
    int rightSidePadding = 8;
    int leftSidePadding = 3;
    int yPos = 3;
    int barWidth = windowWidth - rightSidePadding;

    double progress = displayState.totalSeconds > 0 ? displayState.totalElapsedTime / displayState.totalSeconds : 0;
    double filled = progress * barWidth;

    int fullBlocks = static_cast<int>(filled);
    double remainder = filled - fullBlocks;

    const wchar_t* partials[] = {L"▏", L"▎", L"▍", L"▌", L"▋", L"▊", L"▉"};

    wmove(audioInfoWindow, yPos, 0);
    wclrtoeol(audioInfoWindow);

    wattron(audioInfoWindow, COLOR_PAIR(5));
    mvwaddwstr(audioInfoWindow, yPos, leftSidePadding-1, L"▉");
    mvwaddwstr(audioInfoWindow, yPos, barWidth+leftSidePadding, L"▉");
    wattroff(audioInfoWindow, COLOR_PAIR(5));

    wattron(audioInfoWindow, COLOR_PAIR(4));

    for (int i = 0; i < barWidth; i++) {
        if (i < fullBlocks) {
            mvwaddwstr(audioInfoWindow, yPos, i+leftSidePadding, L"█");
        }
        else if (i == fullBlocks && remainder > 0.1) {
            int block = static_cast<int>(remainder * 7);
            if (block > 6) block = 6;
            mvwaddwstr(audioInfoWindow, yPos, i+leftSidePadding, partials[block]);
        }
    }

    wattroff(audioInfoWindow, COLOR_PAIR(4));

    wmove(audioInfoWindow, yPos+1, leftSidePadding-1);
    wclrtoeol(audioInfoWindow);

    wattron(audioInfoWindow, COLOR_PAIR(4));
    wattron(audioInfoWindow, WA_BOLD);

    wprintw(audioInfoWindow, "Time: %d:%02d/%s",
            displayState.elapsedMinutes, displayState.elapsedSeconds, displayState.duration.c_str());

    wattroff(audioInfoWindow, COLOR_PAIR(4));
    wattroff(audioInfoWindow, WA_BOLD);
}