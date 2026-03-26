#include <locale.h>
#include <cctype>
#include <algorithm>
#include <vector>
#include <string>
#include <chrono>

#include "states.hpp"
#include "tuimanager.hpp"
#include "ncursesw/ncurses.h"

void initializeTerminal() {
    initscr();
    start_color();


    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_WHITE, COLOR_RED);
    init_pair(4, COLOR_GREEN, 0);

    setlocale(LC_ALL, "");

    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
}

void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow) {
    int terminalHeight;
    int terminalWidth;

    getmaxyx(stdscr, terminalHeight, terminalWidth);

    int height{terminalHeight - 4};
    int width{terminalWidth/2 - 1};

    // Split screen vertically into two equal halves
    *fileWindow = newwin(height, width, 0, 0);
    *audioInfoWindow = newwin(height, width, 0, terminalWidth/2);
}

void createBorder(WINDOW** window) {
    cchar_t vline, hline, ulc, urc, llc, lrc;

    setcchar(&vline, L"║", WA_NORMAL, 0, NULL);
    setcchar(&hline, L"═", WA_NORMAL, 0, NULL);
    setcchar(&ulc,   L"╔", WA_NORMAL, 0, NULL);
    setcchar(&urc,   L"╗", WA_NORMAL, 0, NULL);
    setcchar(&llc,   L"╚", WA_NORMAL, 0, NULL);
    setcchar(&lrc,   L"╝", WA_NORMAL, 0, NULL);

    wborder_set(*window, &vline, &vline, &hline, &hline, &ulc, &urc, &llc, &lrc);
}

void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow,
    std::atomic<AudioState>& audioState, AudioState& prevAudioState,
    AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastTime) {

    auto now{std::chrono::steady_clock::now()};
    if (now - lastTime >= std::chrono::milliseconds(150)) {
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

void playNext(WINDOW*& audioInfoWindow, AudioManager& player, std::atomic<AudioState>& audioState, AppState& appState) {
    if (appState.playingIndex < appState.numberOfFiles - 1) appState.playingIndex++;
    else appState.playingIndex = 0;

    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.playingIndex].c_str())};
    player.triggerAudioThread(&audioInfoWindow, &appState, &audioState, audioFilePath);


    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
    appState.isPlaying = true;
    appState.shouldRedraw = true;

    appState.shouldPlayNext = false;
}

void playPrevious(WINDOW *&audioInfoWindow, AudioManager &player, std::atomic<AudioState> &audioState, AppState &appState) {
    if (appState.playingIndex > 0) appState.playingIndex--;
    else appState.playingIndex = appState.numberOfFiles-1;

    char* audioFilePath{const_cast<char*>(appState.audioFiles[appState.playingIndex].c_str())};
    player.triggerAudioThread(&audioInfoWindow, &appState, &audioState, audioFilePath);

    appState.audioName = appState.audioFiles[appState.playingIndex].filename();
    appState.isPlaying = true;
    appState.shouldRedraw = true;

    appState.playPrev = false;
}

std::vector<fs::path> getAudioFiles() {
    fs::path audioPath{AUDIOPP_PATH}; // AUDIOPP_PATH is defined in the CMakeLists.txt

    std::vector<fs::path> files;

    if (!fs::exists(audioPath)) fs::create_directory(audioPath);

    for (const auto entry : fs::directory_iterator(audioPath)) {
        std::string fileExtension{entry.path().extension()};

        // Case-insensitive extension check
        std::transform(fileExtension.begin(), fileExtension.end(),
            fileExtension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (fileExtension == ".wav" || fileExtension == ".flac" || fileExtension == ".ogg" || fileExtension == ".mp3") {
            files.push_back(entry.path());
        }
    }

    return files;
}

void displayFiles(WINDOW*& fileWindow, AppState& appState) {
    int pair;

    for (int i{0}; i < appState.audioFiles.size(); i++) {
        pair = 0;

        if (i == appState.currSelectionIndex) pair = 1;
        else if (appState.isPlaying && i == appState.playingIndex) pair = 2;

        wattron(fileWindow, COLOR_PAIR(pair));
        mvwprintw(fileWindow, i+1, 2, "%d. %s\n", i+1, appState.audioFiles[i].filename().c_str());
        wattroff(fileWindow, COLOR_PAIR(pair));
    }
}

void refreshFiles(WINDOW*& fileWindow, AppState& appState) {
    appState.audioFiles = getAudioFiles();
    appState.numberOfFiles = appState.audioFiles.size();

    // Helps maintain playing highlighter after refresh
    if (appState.isPlaying) {
        for (int i{0}; i < appState.audioFiles.size(); i++) {
            if (appState.audioName == appState.audioFiles[i].filename()) {
                appState.playingIndex = i;
                break;
            }
        }
    }
    werase(fileWindow);

    appState.shouldRefreshFiles = false;
    appState.shouldRedraw = true;
}

void redrawScreen(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, AppState& appState) {

    displayFiles(fileWindow, appState);


    createBorder(&fileWindow);
    createBorder(&audioInfoWindow);

    refresh();
    wrefresh(fileWindow);
    wrefresh(audioInfoWindow);

    appState.shouldRedraw = false;
}
