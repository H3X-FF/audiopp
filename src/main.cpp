#include <iostream>
#include <atomic>
#include <vector>
#include <filesystem>

#include <ncursesw/ncurses.h>

#include "states.hpp"
#include "audiomanager.hpp"
#include "ui.hpp"
#include "commandpipeline.hpp"


int main() {
    bool running{true};
    AppState appState;
    AudioManager player;

    // Threading and State Management
    std::atomic<AudioState> audioState{STOPPED};
    AudioState prevAudioState;

    // Debounce timer for resize events to prevent flickering/crashes
    std::chrono::time_point<std::chrono::steady_clock> lastTime;

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;

    char command[80];

    initializeTerminal();
    initializeWindows(fileWindow, audioInfoWindow);
    initializeAppState(appState);
    initializeAudioDisplayState(appState.audioDisplay);

    while (running) {
        int ch{getch()};

        if (ch != ERR) {
            switch (ch) {
                case 27: // Escape key: Clean shutdown
                    running = false;
                    if (audioState.load() != STOPPED) player.terminateAudioThread();
                    break;

                case ':': // Enter Command Mode
                    nodelay(stdscr, FALSE); // Switch to blocking input
                    echo();
                    move(LINES-1, 0);
                    bkgdset(A_REVERSE);
                    clrtoeol();
                    printw(":");

                    /* Encountered an issue with the enter key leaking into the input,
                     * so this flag is added to block from trying to play audio when in command mode */
                    appState.inCommandMode = true;
                    if (getnstr(command, sizeof(command)-1) == OK) {
                        bkgdset(A_NORMAL);
                        clrtoeol();

                        CommandManager::setUpCommand(command, appState);
                        command[0] = '\0';
                    }

                    nodelay(stdscr, TRUE);
                    noecho();

                    appState.inCommandMode = false;

                    break;

                case '.': // seek forward
                    audioState.store(SEEKING_FWD);
                    break;

                case ',': // seek backwards
                    audioState.store(SEEKING_BWD);
                    break;

                case KEY_RESIZE:
                    // Temporarily pause UI updates in audio thread during resize
                    if (audioState.load() != RESIZING) {
                        prevAudioState = audioState.load();
                        audioState.store(RESIZING);
                    }
                    lastTime = std::chrono::steady_clock::now();
                    appState.shouldResize = true;
                    break;

                case KEY_UP:
                    if (appState.currSelectionIndex > 0) appState.currSelectionIndex--;
                    else appState.currSelectionIndex = appState.audioFiles.size() - 1;
                    appState.shouldRedraw = true;
                    break;

                case KEY_DOWN:
                    if (appState.currSelectionIndex < appState.audioFiles.size()-1) appState.currSelectionIndex++;
                    else appState.currSelectionIndex = 0;
                    appState.shouldRedraw = true;
                    break;

                case 'r': // Manual refresh trigger
                    appState.shouldRefreshFiles = true;
                    break;

                case ' ': // Playback toggle
                    if (audioState.load() == STOPPED) break;
                    appState.isPlaying = !appState.isPlaying;
                    if (audioState.load() == PLAYING) audioState.store(PAUSED);
                    else audioState.store(PLAYING);
                    break;


                case '\n':
                    if (appState.inCommandMode || appState.isPlaying && appState.currSelectionIndex == appState.playingIndex) break;

                    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.currSelectionIndex].c_str())};

                    player.triggerAudioThread(&appState.audioDisplay, &appState, &audioState, audioFilePath);

                    appState.playingIndex = appState.currSelectionIndex;
                    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
                    appState.shouldRedraw = true;

                    break;
            }
        }


        if (appState.shouldResize) resizeWin(fileWindow, audioInfoWindow, audioState, prevAudioState, appState, lastTime);

        if (appState.shouldPlayNext) player.playNext(audioState, appState);

        if (appState.shouldPlayPrev) player.playPrevious(audioState, appState);

        if (appState.shouldRefreshFiles) refreshFiles(fileWindow, appState);

        if (appState.audioDisplay.shouldRedraw) displayAudioInfo(audioInfoWindow, appState.audioDisplay);

        if (appState.shouldRedraw) redrawScreen(fileWindow, audioInfoWindow, appState);
    }

    delwin(fileWindow);
    delwin(audioInfoWindow);
    endwin();

    return 0;
}