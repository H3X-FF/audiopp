#pragma once
#ifndef AUDIOPP_ANIMATIONS_HPP
#define AUDIOPP_ANIMATIONS_HPP

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

#include "states.hpp"

void renderOscilloscope(WINDOW*& audioVisualInfo, AudioDisplayState& displayState);

/** @brief Renders the timer and progress bar */
void renderProgress(WINDOW*& audioInfoWindow, int windowWidth, AudioDisplayState& displayState);

#endif
