#pragma once

#include <ncursesw/ncurses.h>
#include "states.hpp"

void renderOscilloscope(WINDOW*& audioVisualInfo, AudioDisplayState& displayState);

/** @brief Renders the timer and progress bar */
void renderProgress(WINDOW*& audioInfoWindow, int windowWidth, AudioDisplayState& displayState);