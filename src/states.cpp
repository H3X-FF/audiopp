#include "states.hpp"
#include <algorithm>

void initializeAppState(AppState& appState) {
    appState.shouldRedraw = true;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;
    appState.inCommandMode = false;
    appState.shouldPlayNext = false;
    appState.shouldPlayPrev = false;

    appState.topIndex = 0;
    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;

    appState.audioName = "";
}

void initializeAudioDisplayState(AudioDisplayState& audioDisplay) {
    audioDisplay.audioName = "";

    audioDisplay.elapsedMinutes = 0;
    audioDisplay.elapsedSeconds = 0;

    audioDisplay.totalSeconds = 0;
    audioDisplay.totalElapsedTime = 0;
    audioDisplay.amplitude = 0.0;
    audioDisplay.visTimer = 0.0;

    audioDisplay.shouldRedraw = false;

}

std::vector<fs::path> getAudioFiles() {
    fs::path audioPath{AUDIOPP_PATH};

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