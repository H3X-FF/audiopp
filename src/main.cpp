#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <filesystem>

#include "states.h"
#include "audiomanager.h"
#include "tuisetup.h"
#include "ncursesw/ncurses.h"


// #ifndef PROJECT_ASSET_DIR
//     #define PROJECT_ASSET_DIR
// #endif

int main() {

    bool running{true};
    PlayerState playerState;
    AudioManager player;

    std::thread audioThread;
    std::atomic<AudioState> audioState;
    std::vector<fs::path> audioFiles;

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;

    initializeTerminal();
    initializeWindows(&fileWindow, &audioInfoWindow);
    initializePlayerState(playerState);

    while (running) {
        int ch{getch()};

        // if (audioState.load() == FINISHED) {
        //     if (playerState.playingIndex < playerState.numberOfFiles-1) playerState.playingIndex++;
        //     else playerState.playingIndex = 0;
        //
        //     ch = '\n';
        // }

        if (ch != ERR) {
            switch (ch) {
                case 27: // Escape key pressed
                    running = false;

                    if (audioState.load() != STOPPED) {
                        audioState.store(STOPPED);
                        audioThread.join();
                    }
                    break;

                case KEY_UP:
                    if (playerState.currSelectionIndex > 0) playerState.currSelectionIndex--;
                    else playerState.currSelectionIndex = audioFiles.size() - 1;
                    playerState.shouldRedraw = true;
                    break;

                case KEY_DOWN:

                    if (playerState.currSelectionIndex < audioFiles.size()-1) playerState.currSelectionIndex++;
                    else playerState.currSelectionIndex = 0;
                    playerState.shouldRedraw = true;
                    break;

                case 'r': // Refresh the file list
                    playerState.shouldRedraw = true;
                    playerState.shouldRefreshFiles = true;
                    break;

                case ' ': // Pausing and unpausing audio
                    if (audioState.load() == STOPPED) break;

                    playerState.isPlaying = !playerState.isPlaying;

                    if (audioState.load() == PLAYING) audioState.store(PAUSED);
                    else audioState.store(PLAYING);

                    break;


                /* Plays audio and manages the audio thread. It signals a stop first, checks if we can join the thread
                 * so that if there's an active thread that thread exits the loop, resets states
                 * then the new thread comes in and plays the new audio.
                 * The function that plays the audio will modify the state to PLAYING, and it will remain in that loop
                 * as long as the state isn't STOPPED and the track hasn't ended*/
                case '\n':
                    if (playerState.isPlaying && playerState.currSelectionIndex == playerState.playingIndex) break;

                    audioState.store(STOPPED);

                    char* audioPath{const_cast<char*>(audioFiles[playerState.currSelectionIndex].c_str())};

                    if (audioThread.joinable()) audioThread.join();

                    audioThread = std::thread(&AudioManager::playAudio, &player, audioInfoWindow, audioPath, &audioState, &playerState);

                    if (audioState.load() == FAILED) {
                        audioThread.join();

                        attron(COLOR_RED);
                        mvwprintw(audioInfoWindow, 1, 2, "Failed to play audio");
                        attroff(COLOR_RED);

                        refresh();
                        wrefresh(audioInfoWindow);

                        std::this_thread::sleep_for(std::chrono::milliseconds(600));
                    }

                    playerState.audioName = audioFiles[playerState.currSelectionIndex].filename();
                    playerState.playingIndex = playerState.currSelectionIndex;
                    playerState.isPlaying = true;


                    playerState.shouldRedraw = true;

                    break;
            }
        }

        /* Redrawing is requested by the user navigating the files, refreshing the list, or playing audio.
         * It helps highlight the file the user is selecting and the file that's currently playing (if playing audio).
         * Note that refreshing files, while requests a redraw, is its own flag as it can be an expensive operation
         * considering that it has to rescan the files present in the audio folder.
         */
        if (playerState.shouldRedraw) {
            if (playerState.shouldRefreshFiles) {

                audioFiles = getAudioFiles();
                playerState.numberOfFiles = audioFiles.size();
                if (playerState.isPlaying) {
                    for (int i{0}; i < audioFiles.size(); i++) {
                        if (playerState.audioName == audioFiles[i].filename()) {
                            playerState.playingIndex = i;
                            break;
                        }
                    }
                }

                playerState.shouldRefreshFiles = false;
                werase(fileWindow);
            }

            displayFiles(fileWindow ,audioFiles, playerState);

            playerState.shouldRedraw = false;


            createBorder(&fileWindow);
            createBorder(&audioInfoWindow);


            refresh();
            wrefresh(fileWindow);
            wrefresh(audioInfoWindow);
        }

    }

    delwin(fileWindow);
    endwin();

    return 0;
}