#include <iostream>
#include <thread>
#include <string.h>

#include "states.h"
#include "playaudio.h"
#include "ncurses/ncurses.h"


#ifndef PROJECT_ASSET_DIR
    #define PROJECT_ASSET_DIR
#endif

int main() {

    bool running{true};
    PlayerState playerState;
    AudioPlayer player;
    
    initializePlayerState(playerState);

    std::thread audioThread;
    std::atomic<AudioState> audioState;
    std::vector<fs::path> audioFiles{getAudioFiles()};

    initscr();

    if (has_colors()) start_color();

    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);

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

                case 'r':
                case 'R':
                    playerState.shouldRedraw = true;
                    playerState.shouldRefreshFiles = true;
                    break;

                case ' ':
                    if (audioState.load() == STOPPED) break;

                    if (audioState.load() == PLAYING) {
                        playerState.isPlaying = false;
                        audioState.store(PAUSED);
                        move(playerState.playingIndex, playerState.audioName.length()+1);
                        printw("[PAUSED]");
                    }
                    else {
                        playerState.isPlaying = true;
                        audioState.store(PLAYING);
                        move(playerState.playingIndex, playerState.audioName.length()+1);
                        clrtoeol();
                    }

                    break;


                case '\n':
                    if (playerState.isPlaying && playerState.currSelectionIndex == playerState.playingIndex) break;

                    audioState.store(STOPPED);

                    char* audioPath{const_cast<char*>(audioFiles[playerState.currSelectionIndex].c_str())};

                    if (audioThread.joinable()) audioThread.join();

                    audioThread = std::thread(&AudioPlayer::playAudio, &player, audioPath, &audioState, &playerState);

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
                erase();
            }

            displayFiles(audioFiles, playerState);
            refresh();

            playerState.shouldRedraw = false;
        }
    }

    endwin();

    return 0;
}