#include "states.hpp"

void initializeAppState(AppState& appState) {
    appState.isPlaying = false;
    appState.shouldRedraw = true;
    appState.shouldRefreshFiles = true;
    appState.shouldResize = false;
    appState.inCommandMode = false;
    appState.shouldPlayNext = false;

    appState.currSelectionIndex = 0;
    appState.playingIndex = -1;
    appState.numberOfFiles = 0;

    appState.audioName = "";
}