#pragma once
#ifndef AUDIOPP_ANIMATIONS_HPP
#define AUDIOPP_ANIMATIONS_HPP

#include "states.hpp"

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

void renderWaveform(WINDOW*& audioVisualWindow, AppState& appState, std::atomic<AudioState>& audioState);

/** @brief Renders the timer and progress bar */
void renderProgress(WINDOW*& audioInfoWindow, int windowWidth, AudioInfoState& displayState);

#endif
