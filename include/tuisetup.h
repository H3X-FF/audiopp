#ifndef AUDIOPP_TUISETUP_H
#define AUDIOPP_TUISETUP_H

#include "ncursesw/ncurses.h"

void initializeTerminal();
void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow);
void createBorder(WINDOW** window);

#endif //AUDIOPP_TUISETUP_H
