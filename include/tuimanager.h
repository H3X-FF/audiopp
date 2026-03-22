#ifndef AUDIOPP_TUISETUP_H
#define AUDIOPP_TUISETUP_H

#include <filesystem>
#include <vector>
#include "states.h"
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

void initializeTerminal();
void createBorder(WINDOW** window);
void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow);

/* Redrawing is requested by the user navigating the files, refreshing the list, or playing audio.
 * It helps highlight the file the user is selecting and the file that's currently playing (if playing audio).
 * Note that refreshing files, while requests a redraw, is its own flag as it can be an expensive operation
 * considering that it has to rescan the files present in the audio folder.
 */

#endif //AUDIOPP_TUISETUP_H
