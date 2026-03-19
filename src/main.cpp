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
    std::vector<fs::path> audioFiles{getAudioFiles()};

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;

    initializeTerminal();
    initializeWindows(&fileWindow, &audioInfoWindow);
    initializePlayerState(playerState);

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


                case '\n': // Play audio
                    if (playerState.isPlaying && playerState.currSelectionIndex == playerState.playingIndex) break;

                    audioState.store(STOPPED);

                    char* audioPath{const_cast<char*>(audioFiles[playerState.currSelectionIndex].c_str())};

                    if (audioThread.joinable()) audioThread.join();

                    audioThread = std::thread(&AudioManager::playAudio, &player, audioInfoWindow, audioPath, &audioState, &playerState);

                    playerState.audioName = audioFiles[playerState.currSelectionIndex].filename();
                    playerState.playingIndex = playerState.currSelectionIndex;
                    if (!playerState.isPlaying) playerState.isPlaying = true;

                    playerState.shouldRedraw = true;

                    break;
            }
        }

        if (playerState.shouldRedraw) {
            if (playerState.shouldRefreshFiles) {

                audioFiles = getAudioFiles();
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