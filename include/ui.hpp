#pragma once
#ifndef AUDIOPP_UI_HPP
#define AUDIOPP_UI_HPP

#include <atomic>
#include <chrono>

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

#include "states.hpp"

class AudioManager;  // Forward declaration

namespace fs = std::filesystem;

extern const int X_START_POS;

/** @brief Configures ncurses settings (colors, input modes, locale). */
void initializeTerminal();

/** @brief Calculates layout and allocates ncurses WINDOW. */
void initializeWindows(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow);

/** @brief Draws a rounded border using Unicode characters. */
void createBorder(WINDOW*& window);

void scrollList(WINDOW*& fileWindow, AppState& appState);

/** @brief Process resizing with debouncing */
void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow,
    AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastResizeTime);

/** @brief Renders the list of files to the file window with highlighting and determines the top element for scrolling. */
void drawFileList(WINDOW*& fileWindow, AppState& appState);

void drawScreen(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AppState& appState);

/** @brief Reconstructs the audioFiles container. */
void refreshFiles(WINDOW*& fileWindow, AppState& appState);

/** @brief Renders audio info from AudioDisplayState to the audio info window. */
void displayAudioInfo(WINDOW*& audioInfoWindow, AudioInfoState& displayState);

/** @brief Renders the progress bar and visual */
void renderAnimations(WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AudioInfoState& displayState);

void displayVolAndRepeatMode(WINDOW*& audioInfoWindow, AppState& appState, AudioInfoState& displayState);

void cleanupAudioWindows(WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AudioInfoState& displayState);

#endif