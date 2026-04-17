#include "states.hpp"
#include <algorithm>

#include "sort_types.hpp"

void initializeAppState(AppState& appState) {
    appState.shouldRedraw = true;
    appState.shouldCheckForScroll = false;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;
    appState.inCommandMode = false;
    appState.shouldPlayNext = false;
    appState.shouldPlayPrev = false;

    appState.repeatMode = RepeatModes::REPEAT_ALL;

    appState.topIndex = 0;
    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;

    appState.sortActions.sortType = "name";
}

void initializeAudioDisplayState(AudioDisplayState& audioDisplay) {
    audioDisplay.audioName = "";

    audioDisplay.elapsedMinutes = 0;
    audioDisplay.elapsedSeconds = 0;

    audioDisplay.totalSeconds = 0;
    audioDisplay.totalElapsedTime = 0;
    audioDisplay.amplitude = 0.0;
    audioDisplay.visTimer = 0.0;

    audioDisplay.shouldDrawAudioInfo = false;
    audioDisplay.shouldRenderAnimation = false;
    audioDisplay.displayCurrRepeatMode = false;
    audioDisplay.shouldCleanup = false;

    audioDisplay.repeatModeInfo = "All";
}

void getAudioFiles(AppState& appState) {
    fs::path audioPath{AUDIOPP_PATH};

    if (!fs::exists(audioPath)) fs::create_directories(audioPath);

    for (const auto entry : fs::directory_iterator(audioPath)) {
        std::string fileExtension{entry.path().extension()};

        // Case-insensitive extension check
        std::transform(fileExtension.begin(), fileExtension.end(),
            fileExtension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (fileExtension == ".wav" || fileExtension == ".flac" || fileExtension == ".mp3") {
            appState.audioFiles.push_back(entry.path());
        }
    }

    if (appState.sortActions.sortType == "name") {
        sortByName(appState);
    }
    else if (appState.sortActions.sortType == "lwt") {
        sortByLastWrite(appState);
    }
    else if (appState.sortActions.sortType == "size") {
        sortBySize(appState);
    }
    else if (appState.sortActions.sortType == "ext") {
        sortByExtension(appState);
    }
}