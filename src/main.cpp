#include <iostream>
#include <atomic>
#include <vector>
#include <filesystem>
#include <signal.h>
#include <unistd.h>

#include <ncursesw/ncurses.h>

#include "states.hpp"
#include "audiomanager.hpp"
#include "ui.hpp"
#include "command_mode_and_pipe.hpp"

#define ESCAPE_KEY 27
#define ENTER_KEY '\n'

int main() {
    bool running = true;
    AppState appState;
    AudioManager player;

    // Threading and State Management
    std::atomic<AudioState> audioState{AudioState::STOPPED};


    // Debounce timer for resize events to prevent flickering/crashes
    std::chrono::time_point<std::chrono::steady_clock> lastTime;

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;
    WINDOW* audioVisualWindow;

    initializeTerminal();
    initializeWindows(fileWindow, audioInfoWindow, audioVisualWindow);
    initializeAppState(appState);
    initializeAudioDisplayState(appState.audioDisplayState);

    while (running) {

        int ch = getch();

        if (ch != ERR) {
            switch (ch) {
                case 'q':
                case ESCAPE_KEY: {
                    move(LINES-1, 0);
                    clrtoeol();
                    printw("Exit? [y/n]");

                    int exitCh;
                    bool shouldExit = false;

                    while (true) {
                        exitCh = getch();

                        if (exitCh == 'y') {
                            shouldExit = true;
                            break;
                        }

                        if (exitCh == 'n') break;
                    }

                    if (shouldExit) {
                        running = false;
                        if (audioState.load() != AudioState::STOPPED) player.terminateAudioThread();
                    }

                    move(LINES-1, 0);
                    clrtoeol();

                    break;
                }

                case KEY_UP: {
                    if (appState.numberOfFiles == 0) break;

                    appState.currSelectionIndex = (appState.currSelectionIndex - 1 + appState.numberOfFiles) % appState.numberOfFiles;

                    appState.shouldCheckForScroll = true;

                    appState.shouldRedraw = true;
                    break;
                }

                case KEY_DOWN: {
                    if (appState.numberOfFiles == 0) break;

                    appState.currSelectionIndex = (appState.currSelectionIndex + 1) % appState.numberOfFiles;

                    appState.shouldCheckForScroll = true;

                    appState.shouldRedraw = true;
                    break;
                }

                case KEY_RESIZE:
                    lastTime = std::chrono::steady_clock::now();
                    // calls resizeWin() to recalculate the new size, and prevents audio windows from displaying anything
                    appState.shouldResize = true;

                    break;

                case 'r': // Manual refresh trigger
                    appState.shouldRefreshFiles = true;
                    break;

                case ' ': // Playback toggle
                    if (audioState.load() == AudioState::STOPPED) break;

                    if (audioState.load() == AudioState::PLAYING) audioState.store(AudioState::PAUSED);
                    else audioState.store(AudioState::PLAYING);

                    break;

                case 'f': {
                    if (appState.playingIndex == -1)  break;

                    appState.currSelectionIndex = appState.playingIndex;

                    appState.shouldCheckForScroll = true;

                    appState.shouldRedraw = true;

                    break;
                }

                case 'l':
                    appState.repeatMode = static_cast<RepeatModes>((static_cast<int>(appState.repeatMode) + 1) % 3);

                    appState.shouldChangeRepeatMode = true;

                    break;

                case ':': {
                    // Can't risk corruption in the buffer and cursor during command mode!
                    sigset_t set;
                    sigemptyset(&set);
                    sigaddset(&set, SIGWINCH);

                    sigprocmask(SIG_BLOCK, &set, NULL);

                    commandMode(appState);

                    sigprocmask(SIG_UNBLOCK, &set, NULL);

                    break;
                }

                case '.': // seek forward
                    if (audioState.load() == AudioState::PAUSED) player.wasPaused = true;
                    audioState.store(AudioState::SEEKING_FWD);
                    break;

                case ',': // seek backwards
                    if (audioState.load() == AudioState::PAUSED) player.wasPaused = true;
                    audioState.store(AudioState::SEEKING_BWD);
                    break;


                case ENTER_KEY:
                    if (appState.numberOfFiles == 0 ||appState.inCommandMode ||
                        appState.currSelectionIndex == appState.playingIndex) break;

                    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.currSelectionIndex].c_str())};

                    player.triggerAudioThread(&appState.audioDisplayState, &appState, &audioState, audioFilePath);

                    appState.playingIndex = appState.currSelectionIndex;
                    appState.audioDisplayState.audioName = appState.audioFiles[appState.playingIndex].filename();
                    appState.shouldRedraw = true;

                    break;
            }
        }


        if (appState.shouldResize) resizeWin(fileWindow, audioInfoWindow, audioVisualWindow, appState, lastTime);

        if (appState.shouldRedraw) redrawScreen(fileWindow, audioInfoWindow, audioVisualWindow, appState);

        if (appState.shouldCheckForScroll) scrollList(fileWindow, appState);

        if (appState.shouldPlayNext) player.playNext(audioState, appState);

        if (appState.shouldPlayPrev) player.playPrevious(audioState, appState);

        if (appState.shouldRefreshFiles) refreshFiles(fileWindow, appState);

        if (appState.audioDisplayState.shouldDrawAudioInfo) displayAudioInfo(
            audioInfoWindow, appState.audioDisplayState);

        if (appState.audioDisplayState.shouldRenderAnimation) renderAnimations(
            audioInfoWindow, audioVisualWindow, appState.audioDisplayState);

        if (appState.audioDisplayState.displayCurrRepeatMode) displayRepeatMode(
            audioInfoWindow, appState.audioDisplayState);

        if (appState.audioDisplayState.shouldCleanup) cleanupAudioWindows(
            audioInfoWindow, audioVisualWindow, appState.audioDisplayState);
    }

    delwin(fileWindow);
    delwin(audioInfoWindow);
    delwin(audioVisualWindow);
    endwin();

    return 0;
}