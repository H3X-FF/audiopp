#pragma once

#include <filesystem>
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

/** @brief Configures ncurses settings (colors, input modes, locale). */
void initializeTerminal();

/** @brief Draws double-line borders using wide-character support. */
void createBorder(WINDOW** window);

/** @brief Calculates layout and allocates ncurses WINDOW pointers. */
void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow);