#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <filesystem>

#include "states.h"
#include "audiomanager.h"
#include "tuimanager.h"
#include "ncursesw/ncurses.h"

int main() {

    bool running{true};
    AppState appState;
    AudioManager player;

    std::thread audioThread;
    std::atomic<AudioState> audioState;
    std::vector<fs::path> audioFiles;

    AudioState prevAudioState;

    std::chrono::time_point<std::chrono::steady_clock> lastResizeTime;
    const std::chrono::milliseconds DEBOUNCE_PERIOD{150};

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;

    initializeTerminal();
    initializeWindows(&fileWindow, &audioInfoWindow);
    initializeAppState(appState);

    while (running) {
        int ch{getch()};

        if (ch != ERR) {
            switch (ch) {
                case 27: // Escape key pressed
                    running = false;

                    if (audioState.load() != STOPPED) {
                        audioState.store(STOPPED);
                        audioThread.join();
                    }
                    break;

                case KEY_RESIZE:
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

                case 'r': // Refresh the file list
                    appState.shouldRedraw = true;
                    appState.shouldRefreshFiles = true;
                    break;

                case ' ': // Pausing and unpausing audio
                    if (audioState.load() == STOPPED) break;

                    appState.isPlaying = !appState.isPlaying;

                    if (audioState.load() == PLAYING) audioState.store(PAUSED);
                    else audioState.store(PLAYING);

                    break;


                /* Plays audio and manages the audio thread. It signals a stop first, checks if we can join the thread
                 * so that if there's an active thread that thread exits the loop, resets states
                 * then the new thread comes in and plays the new audio.
                 * The function that plays the audio will modify the state to PLAYING, and it will remain in that loop
                 * as long as the state isn't STOPPED and the track hasn't ended */
                case '\n':
                    if (appState.isPlaying && appState.currSelectionIndex == appState.playingIndex) break;

                    audioState.store(STOPPED);

                    char* audioPath{const_cast<char*>(audioFiles[appState.currSelectionIndex].c_str())};

                    if (audioThread.joinable()) audioThread.join();

                    audioThread = std::thread(&AudioManager::playAudio, &player, &audioInfoWindow, audioPath, &audioState, &appState);

                    if (audioState.load() == FAILED) {
                        audioThread.join();

                        attron(COLOR_RED);
                        mvwprintw(audioInfoWindow, 1, 2, "Failed to play audio");
                        attroff(COLOR_RED);

                        refresh();
                        wrefresh(audioInfoWindow);

                        std::this_thread::sleep_for(std::chrono::milliseconds(600));
                    }

                    appState.audioName = audioFiles[appState.currSelectionIndex].filename();
                    appState.playingIndex = appState.currSelectionIndex;
                    appState.isPlaying = true;


                    appState.shouldRedraw = true;

                    break;
            }
        }

        if (appState.shouldResize) {
            auto now{std::chrono::steady_clock::now()};

            if (now - lastResizeTime >= DEBOUNCE_PERIOD) {
                if (fileWindow) delwin(fileWindow);
                if (audioInfoWindow) delwin(audioInfoWindow);

                /* Forces ncurses to discard what it knows about the terminal to help with an issue with COLS and LINES.
                 * I couldn't get resizing to work properly, there always was crashes and undefined behavior, so this is
                 * pretty much the solution I found that helps with the issues I kept running into. */
                endwin();
                refresh();
                clear();

                initializeWindows(&fileWindow, &audioInfoWindow);

                appState.shouldResize = false;
                appState.shouldRedraw = true;
                audioState.store(prevAudioState);
            }
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