#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <filesystem>

#include "states.hpp"
#include "audiomanager.hpp"
#include "tuimanager.hpp"
#include "ncursesw/ncurses.h"
#include "commandpipeline.hpp"



int main() {
    bool running{true};
    AppState appState;
    AudioManager player;

    // Threading and State Management
    std::thread audioThread;
    std::atomic<AudioState> audioState;
    AudioState prevAudioState;

    std::vector<fs::path> audioFiles;


    // Debounce timer for resize events to prevent flickering/crashes
    std::chrono::time_point<std::chrono::steady_clock> lastResizeTime;
    const std::chrono::milliseconds DEBOUNCE_PERIOD{150};

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;

    char command[80];

    initializeTerminal();
    initializeWindows(&fileWindow, &audioInfoWindow);
    initializeAppState(appState);

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
                    lastResizeTime = std::chrono::steady_clock::now();
                    appState.shouldResize = true;
                    break;

                case KEY_UP:
                    if (appState.currSelectionIndex > 0) appState.currSelectionIndex--;
                    else appState.currSelectionIndex = audioFiles.size() - 1;
                    appState.shouldRedraw = true;
                    break;

                case KEY_DOWN:
                    if (appState.currSelectionIndex < audioFiles.size()-1) appState.currSelectionIndex++;
                    else appState.currSelectionIndex = 0;
                    appState.shouldRedraw = true;
                    break;

                case 'r': // Manual refresh trigger
                    appState.shouldRedraw = true;
                    appState.shouldRefreshFiles = true;
                    break;

                case ' ': // Playback toggle
                    if (audioState.load() == STOPPED) break;
                    appState.isPlaying = !appState.isPlaying;
                    if (audioState.load() == PLAYING) audioState.store(PAUSED);
                    else audioState.store(PLAYING);
                    break;

                /*
                * Plays audio and manages the audio thread. It signals a stop first, checks if we can join the thread
                * so that if there's an active thread that thread exits the loop, resets states
                * then the new thread comes in and plays the new audio.
                * The function that plays the audio will modify the state to PLAYING, and it will remain in that loop
                * as long as the state isn't STOPPED and the track hasn't ended
                */
                case '\n':
                    if (appState.inCommandMode || appState.isPlaying && appState.currSelectionIndex == appState.playingIndex) break;

                    char* audioFilePath{const_cast<char*>(audioFiles[appState.currSelectionIndex].c_str())};

                    player.triggerAudioThread(&audioInfoWindow, &appState, &audioState, audioFilePath);

                    appState.playingIndex = appState.currSelectionIndex;
                    appState.audioName = audioFiles[appState.playingIndex].filename();
                    appState.isPlaying = true;
                    appState.shouldRedraw = true;
                    break;
            }
        }

        // Process resizing with debouncing
        if (appState.shouldResize) {
            auto now{std::chrono::steady_clock::now()};
            if (now - lastResizeTime >= DEBOUNCE_PERIOD) {
                if (fileWindow) delwin(fileWindow);
                if (audioInfoWindow) delwin(audioInfoWindow);

                // Hard reset ncurses to recalculate internal terminal dimensions
                endwin();
                refresh();
                clear();

                initializeWindows(&fileWindow, &audioInfoWindow);

                appState.shouldResize = false;
                appState.shouldRedraw = true;
                audioState.store(prevAudioState);
            }
        }

        if (appState.playNext) {
            if (appState.playingIndex < appState.numberOfFiles - 1) appState.playingIndex++;
            else appState.playingIndex = 0;

            char* audioFilePath{const_cast<char*>(audioFiles[appState.playingIndex].c_str())};
            player.triggerAudioThread(&audioInfoWindow, &appState, &audioState, audioFilePath);


            appState.audioName = audioFiles[appState.playingIndex].filename();
            appState.isPlaying = true;
            appState.shouldRedraw = true;

            appState.playNext = false;
        }

        if (appState.playPrev) {
            if (appState.playingIndex > 0) appState.playingIndex--;
            else appState.playingIndex = appState.numberOfFiles-1;

            char* audioFilePath{const_cast<char*>(audioFiles[appState.playingIndex].c_str())};
            player.triggerAudioThread(&audioInfoWindow, &appState, &audioState, audioFilePath);


            appState.audioName = audioFiles[appState.playingIndex].filename();
            appState.isPlaying = true;
            appState.shouldRedraw = true;

            appState.playPrev = false;
        }

        /* Redrawing is requested by the user navigating the files, refreshing the list, or playing audio.
         * It helps highlight the file the user is selecting and the file that's currently playing (if playing audio).
         * Note that refreshing files, while requests a redraw, is its own flag as it can be an expensive operation
         * considering that it has to rescan the files present in the audio folder.
         */
        if (appState.shouldRedraw) {
            if (appState.shouldRefreshFiles) {
                audioFiles = getAudioFiles();
                appState.numberOfFiles = audioFiles.size();

                // Helps maintain playing highlighter after refresh
                if (appState.isPlaying) {
                    for (int i{0}; i < audioFiles.size(); i++) {
                        if (appState.audioName == audioFiles[i].filename()) {
                            appState.playingIndex = i;
                            break;
                        }
                    }
                }
                appState.shouldRefreshFiles = false;
                werase(fileWindow);
            }

            displayFiles(fileWindow ,audioFiles, appState);
            appState.shouldRedraw = false;

            createBorder(&fileWindow);
            createBorder(&audioInfoWindow);

            refresh();
            wrefresh(fileWindow);
            wrefresh(audioInfoWindow);
        }
    }

    delwin(fileWindow);
    delwin(audioInfoWindow);
    endwin();

    return 0;
}