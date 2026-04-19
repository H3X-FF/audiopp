#pragma once
#ifndef AUDIOPP_ANIMATIONS_HPP
#define AUDIOPP_ANIMATIONS_HPP

#include <ncursesw/ncurses.h>
#include "states.hpp"

void renderOscilloscope(WINDOW*& audioVisualInfo, AudioDisplayState& displayState);

/** @brief Renders the timer and progress bar */
void renderProgress(WINDOW*& audioInfoWindow, int windowWidth, AudioDisplayState& displayState);

#endif
