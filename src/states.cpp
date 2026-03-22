#include <vector>
#include <filesystem>
#include <cstdlib>
#include <string>

#include "states.h"
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

static const char* APP_PATH{std::getenv("HOME")};

std::string getHomeDir() {
    static const std::string homedir{[]() {
        const char* h{std::getenv("HOME")};
        return h ? std::string(h) : "";
    }()};

    return homedir;
};

void initializeAppState(AppState& appState) {
    appState.isPlaying = false;
    appState.shouldRedraw = true;
    appState.shouldRefreshFiles = true;
    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;
    appState.audioName = "";
    appState.duration = "";
}

std::vector<fs::path> getAudioFiles() {
    fs::path appPath{getHomeDir()};
    appPath /= ".local";

    fs::path audioPath{appPath/"audiopp/audio"};

    std::vector<fs::path> files;

    if (!fs::is_directory(audioPath)) fs::create_directories(audioPath);

    for (const auto entry : fs::directory_iterator(appPath)) {
        if (entry.path().extension() == ".wav") {
            files.push_back(entry.path());
        }
    }

    return files;
}

void displayFiles(WINDOW* fileWindow, std::vector<fs::path> audioFiles, AppState appState) {

    int pair;

    for (int i{0}; i < audioFiles.size(); i++) {
        pair = 0;

        if (i == appState.currSelectionIndex) pair = 1;
        else if (appState.isPlaying && i == appState.playingIndex) pair = 2;

        wattron(fileWindow, COLOR_PAIR(pair));
        mvwprintw(fileWindow, i+1, 2, "%s\n", audioFiles[i].filename().c_str());
        wattroff(fileWindow, COLOR_PAIR(pair));
    }
}