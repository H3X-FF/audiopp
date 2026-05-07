#include <string>
#include <vector>
#include <sstream>

#include "help.hpp"
#include "states.hpp"

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

#define ESCAPE_KEY 27

namespace {
    void createHelpWindow(WINDOW*& helpWindow) {
        int termHeight, termWidth;
        getmaxyx(stdscr, termHeight, termWidth);

        int xPadding = 3;
        int yPadding = 1;

        int windowHeight = termHeight - yPadding;
        int windowWidth = termWidth - xPadding;


        int yPos = (termHeight - windowHeight)/2;
        int xPos = (termWidth - windowWidth)/2;

        helpWindow = newwin(windowHeight, windowWidth, yPos, xPos);
        scrollok(helpWindow, TRUE);

        refresh();
        wrefresh(helpWindow);
    }

    void printWrap(WINDOW*& win, std::string msg) {
        int winWidth = getmaxx(win);

        std::stringstream ss(msg);
        std::string word;

        int currY, currX;

        wprintw(win, " ");
        while (ss >> word) {
            getyx(win, currY, currX);
            int wordLen = word.length();

            if (currX + wordLen >= winWidth) {
                wmove(win, currY+1, 2);
            }

            wprintw(win, "%s ", word.c_str());
        }
    }
}

namespace {
    void displayHelpForKeys(WINDOW*& helpWindow) {
        std::vector<std::pair<std::string, std::string>> keysHelp{
            {"Up Arrow", "Go up in the list"},
            {"Down Arrow", "Go down in the list"},
            {"ENTER", "Start track"},
            {"SPACE", "Play/Pause active track"},
            {",", "Seek backward"},
            {".", "Seek forward"},
            {"[", "Volume down"},
            {"]", "Volume up"},
            {"r", "Manually refresh file list"},
            {"f", "Go to active track in the list"},
            {"l", "Change repeat mode"},
            {":", "Enter command mode"},
            {"ESC", "Exit command mode/program"},
        };

        int xPadding = 2;
        int yPos = 1;

        int winHeight, winWidth;
        getmaxyx(helpWindow, winHeight, winWidth);

        mvwprintw(helpWindow, yPos, xPadding, "---KEYS---");

        size_t longestDesc = keysHelp[0].second.size();

        for (int i = 0; i < keysHelp.size(); i++) {
            yPos+=2;

            wattron(helpWindow, A_REVERSE);
            mvwprintw(helpWindow, yPos, xPadding, " %s ", keysHelp[i].first.c_str());
            wattroff(helpWindow, A_REVERSE);

            wprintw(helpWindow, "%s ", keysHelp[i].second.c_str());
            if (keysHelp[i].second.size() > longestDesc) {
                longestDesc = keysHelp[i].second.size();

                if (yPos >= winHeight - 5) {
                    xPadding += longestDesc + 10;
                    yPos = 2;
                }

            }

        }

    //-------------------------------------------------------

        std::string txt = "NEXT>>>";

        int txtPosX = (winWidth - txt.length()) - 1;

        // wmove(helpWindow, winHeight-1, txtPosX);
        // clrtoeol();
        wattron(helpWindow, A_REVERSE);
        mvwprintw(helpWindow, winHeight-1, txtPosX, "%s", txt.c_str());
        wattroff(helpWindow, A_REVERSE);


        refresh();
        wrefresh(helpWindow);
    }


    void displayHelpForCmd(WINDOW*& helpWindow) {
        std::vector<std::pair<std::string, std::string>> cmdHelp {
            {"scan [DIR]", "scans directory and adds them to the list. Flags:  --recurse"},
            {"rm [filename/index]", "removes track. Surround names that have spaces with \'"},
            {"rename [filename/index] [new name]", "Changes file name. Surround names that have spaces with \'"},
            {"sort [sort type]", "sort types: name, lwt(last write time), size, ext (extension)"
                                 " Flags: --normal (default) --reverse"}
        };

        const int xPadding = 2;
        int yPos = 1;

        mvwprintw(helpWindow, yPos, xPadding, "---COMMANDS---");


        for (int i = 0; i < cmdHelp.size(); i++) {
            yPos+=2;

            wattron(helpWindow, A_REVERSE);
            mvwprintw(helpWindow, yPos, xPadding, "%s", cmdHelp[i].first.c_str());
            wattroff(helpWindow, A_REVERSE);

            printWrap(helpWindow, cmdHelp[i].second);
        }

    //---------------------------------------------------------
        int winHeight, winWidth;
        getmaxyx(helpWindow, winHeight, winWidth);

        std::string txt = "<<<PREV";

        wattron(helpWindow, A_REVERSE);
        mvwprintw(helpWindow, winHeight-2, xPadding, "%s", txt.c_str());
        wattroff(helpWindow, A_REVERSE);

        refresh();
        wrefresh(helpWindow);

    }
}

void displayHelp(AppState& appState) {
    enum class HelpScreens {
        HELP_KEY,
        HELP_CMD
    };

    WINDOW* helpWindow;

    HelpScreens helpScr = HelpScreens::HELP_KEY;

    createHelpWindow(helpWindow);
    displayHelpForKeys(helpWindow);

    int ch;

    bool resize = false;
    int winWidth, winHeight;
    getmaxyx(helpWindow, winWidth, winHeight);

    while (true) {
        ch = getch();

        if (ch == 'h' || ch == 'q' || ch == ESCAPE_KEY) break;

        if (helpScr == HelpScreens::HELP_KEY && ch == KEY_RIGHT) {
            helpScr = HelpScreens::HELP_CMD;

            werase(helpWindow);
            displayHelpForCmd(helpWindow);

            refresh();
            wrefresh(helpWindow);
        }

        if (helpScr == HelpScreens::HELP_CMD && ch == KEY_LEFT) {
            helpScr = HelpScreens::HELP_KEY;

            werase(helpWindow);
            displayHelpForKeys(helpWindow);

            refresh();
            wrefresh(helpWindow);
        }

        if (ch == KEY_RESIZE) {
            if (helpWindow) delwin(helpWindow);

            createHelpWindow(helpWindow);


            if (helpScr == HelpScreens::HELP_KEY) {
                werase(helpWindow);
                displayHelpForKeys(helpWindow);
            }
            else {
                werase(helpWindow);
                displayHelpForCmd(helpWindow);
            }

            refresh();
            wrefresh(helpWindow);
            // resize main UI after exiting the help screen so that it stays accurate to the terminal size
            appState.shouldResize = true;
        }

    }

    delwin(helpWindow);

    appState.shouldRedrawScreen = true;

    // Avoid writing text while nothing, but display it if something is playing after exiting the help screen
    if (appState.playingIndex != -1) {
        appState.audioDisplayState.shouldDrawAudioInfo = true;
        appState.audioDisplayState.shouldUpdateVolOrRepeatTxt = true;
    }
}