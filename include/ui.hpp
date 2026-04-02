#pragma once

#include <atomic>
#include <chrono>

#include <ncursesw/ncurses.h>

#include "states.hpp"

class AudioManager;  // Forward declaration

namespace fs = std::filesystem;

/** @brief Configures ncurses settings (colors, input modes, locale). */
void initializeTerminal();

/** @brief Calculates layout and allocates ncurses WINDOW. */
void initializeWindows(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow);

/** @brief Draws a rounded border using Unicode characters. */
void createBorder(WINDOW*& window);

/** @brief Process resizing with debouncing */
void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow,
    std::atomic<AudioState>& audioState, AudioState& prevAudioState,
    AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastTime);

/** @brief Renders the list of files to the file window with highlighting and determines the top element for scrolling. */
void displayFiles(WINDOW*& fileWindow, AppState& appState);

/** @brief Used to update the UI */
void redrawScreen(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AppState& appState);

/** @brief Reconstructs the audioFiles container. */
void refreshFiles(WINDOW*& fileWindow, AppState& appState);

/** @brief Renders audio info from AudioDisplayState to the audio info window. */
void displayAudioInfo(WINDOW*& audioInfoWindow, WINDOW*& audioVisualWindow, AudioDisplayState& displayState);