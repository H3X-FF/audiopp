#include <iostream>
#include <atomic>
#include <vector>
#include <signal.h>

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif


#include "states.hpp"
#include "audiomanager.hpp"
#include "ui.hpp"
#include "command_mode_and_pipe.hpp"
#include "help.hpp"
#include "vfs.hpp"
#include "settings.hpp"

#define ESCAPE_KEY 27
#define ENTER_KEY '\n'

int main() {
    bool running = true;
    AppState appState;

    // Threading and State Management
    std::atomic<AudioState> audioState{AudioState::STOPPED};

    // Debounce timer for resize events to prevent flickering/crashes
    std::chrono::time_point<std::chrono::steady_clock> lastResizeTime;

    WINDOW* fileWindow;
    WINDOW* audioInfoWindow;
    WINDOW* audioVisualWindow;

    initializeTerminal();
    initializeWindows(fileWindow, audioInfoWindow, audioVisualWindow);

    initializeAppState(appState);
    initializeAudioDisplayState(appState.audioDisplayState);

    loadSettings(appState);

    AudioManager player(&appState.audioDisplayState, &appState, &audioState);

    //---Program loop---
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
                        // if (audioState.load() != AudioState::STOPPED) player.terminateAudioThread();
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
                    lastResizeTime = std::chrono::steady_clock::now();

                    // calls resizeWin() to recalculate the new size, and prevents audio windows from displaying anything
                    appState.shouldResize = true;

                    break;

                case 'h':
                    werase(fileWindow);
                    werase(audioInfoWindow);
                    werase(audioVisualWindow);

                    refresh();
                    wrefresh(fileWindow);
                    wrefresh(audioInfoWindow);
                    wrefresh(audioVisualWindow);
                    displayHelp(appState);

                    break;

                case 'r': // Manual refresh trigger
                    appState.shouldRefreshFiles = true;
                    break;

                case ' ': // Playback toggle
                    if (audioState.load() == AudioState::STOPPED) break;

                    if (audioState.load() == AudioState::PLAYING) audioState.store(AudioState::PAUSED);
                    else audioState.store(AudioState::PLAYING);

                    break;

                case 'f':
                    if (appState.playingIndex == -1)  break;

                    appState.currSelectionIndex = appState.playingIndex;

                    appState.shouldCheckForScroll = true;

                    appState.shouldRedraw = true;

                    break;


                case 'l':
                    appState.repeatMode = static_cast<RepeatModes>((static_cast<int>(appState.repeatMode) + 1) % 3);

                    appState.audioDisplayState.shouldUpdateVolOrRepeatTxt = true;

                    break;

                case ':': {
                    #ifdef _WIN32
                        // I honestly tried to block resizing on Windows but couldn't manage to get any of them to work
                        commandMode(appState);
                    #else
                        // Can't risk corruption in the buffer and cursor during command mode!
                        sigset_t set;
                        sigemptyset(&set);
                        sigaddset(&set, SIGWINCH);

                        sigprocmask(SIG_BLOCK, &set, NULL);

                        commandMode(appState);

                        sigprocmask(SIG_UNBLOCK, &set, NULL);
                    #endif

                    break;
                }

                case ',': // seek backwards
                    if (audioState.load() == AudioState::PAUSED) player.wasPaused = true;
                    audioState.store(AudioState::SEEKING_BWD);
                    break;

                case '.': // seek forward
                    if (audioState.load() == AudioState::PAUSED) player.wasPaused = true;
                    audioState.store(AudioState::SEEKING_FWD);
                    break;

                case '[': // volume down
                    if (audioState.load() != AudioState::STOPPED) {
                        float currentVol = player.volumeSlider.load(std::memory_order_relaxed);
                        float newVol = std::max(0.0f, currentVol - 0.05f);
                        player.volumeSlider.store(newVol, std::memory_order_relaxed);

                        appState.audioDisplayState.volume = newVol;
                        appState.audioDisplayState.shouldUpdateVolOrRepeatTxt = true;
                    }
                    break;

                case ']': // volume up
                    if (audioState.load() != AudioState::STOPPED) {
                        float currentVol = player.volumeSlider.load(std::memory_order_relaxed);
                        float newVol = std::min(1.0f, currentVol + 0.05f);
                        player.volumeSlider.store(newVol, std::memory_order_relaxed);

                        appState.audioDisplayState.volume = newVol;
                        appState.audioDisplayState.shouldUpdateVolOrRepeatTxt = true;
                    }
                    break;



                case ENTER_KEY:
                    if (appState.numberOfFiles == 0 ||appState.inCommandMode ||
                        appState.currSelectionIndex == appState.playingIndex) break;

                    char* audioFilePath = const_cast<char*>(appState.vfs.audioMap[appState.vfs.audioFileNames[appState.currSelectionIndex]].c_str());

                    appState.playingIndex = appState.currSelectionIndex;
                    // appState.audioDisplayState.audioName = appState.vfs.audioFileNames[appState.playingIndex];
                    player.triggerAudioThread(audioFilePath);
                    break;
            }
        }


        // Just a bunch of state checks
        if (appState.shouldCheckForScroll) {
            scrollList(fileWindow, appState);
        }

        if (appState.shouldRedraw) {
            redrawScreen(fileWindow, audioInfoWindow, audioVisualWindow, appState);
        }

        if (appState.shouldRefreshFiles) {
            refreshFiles(fileWindow, appState);
        }

        if (appState.shouldResize) {
            resizeWin(fileWindow, audioInfoWindow, audioVisualWindow, appState, lastResizeTime);
        }

        if (appState.audioDisplayState.shouldRenderAnimation) {
            renderAnimations(audioInfoWindow, audioVisualWindow, appState.audioDisplayState);
        }

        if (appState.audioDisplayState.shouldDrawAudioInfo) {
            displayAudioInfo(audioInfoWindow, appState.audioDisplayState);
        }

        if (appState.audioDisplayState.shouldUpdateVolOrRepeatTxt) {
            displayVolAndRepeatMode(audioInfoWindow, appState, appState.audioDisplayState);
        }

        if (appState.audioDisplayState.shouldCleanup) {
            cleanupAudioWindows(audioInfoWindow, audioVisualWindow, appState.audioDisplayState);
        }

        if (appState.shouldPlayNext) {
            player.playNext(audioState, appState);
        }

        if (appState.shouldPlayPrev) {
            player.playPrevious(audioState, appState);
        }

        if (audioState.load() == AudioState::FAILED) {
            player.terminateAudioThread();
            printError("Failed to initialize device");
        }

    }

    saveSettings(appState);

    delwin(fileWindow);
    delwin(audioInfoWindow);
    delwin(audioVisualWindow);
    endwin();

    return 0;
}