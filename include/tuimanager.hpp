#pragma once

#include <filesystem>
#include <atomic>
#include <vector>
#include <chrono>

#include "audiomanager.hpp"
#include "states.hpp"
#include "ncursesw/ncurses.h"

namespace fs = std::filesystem;

/** @brief Configures ncurses settings (colors, input modes, locale). */
void initializeTerminal();

/** @brief Draws double-line borders using wide-character support. */
void createBorder(WINDOW** window);

/** @brief Calculates layout and allocates ncurses WINDOW pointers. */
void initializeWindows(WINDOW** fileWindow, WINDOW** audioInfoWindow);

/** @brief Sets default values for the AppState. */
void initializeAppState(AppState& appState);

/** @brief Scans the designated audio directory for audio files. */
std::vector<fs::path> getAudioFiles();

/** @brief Renders the list of files to the file window with highlighting. */
void displayFiles(WINDOW*& fileWindow, AppState& appState);

/** @brief Process resizing with debouncing */
void resizeWin(WINDOW*& fileWindow, WINDOW*& audioInfoWindow,
    std::atomic<AudioState>& audioState, AudioState& prevAudioState,
    AppState& appState, std::chrono::time_point<std::chrono::steady_clock>& lastTime);

/** @brief Automatically play next audio */
void playNext(WINDOW*& audioInfoWindow, AudioManager& player, std::atomic<AudioState>& audioState, AppState& appState);

/** @brief Automatically play previous audio */
void playPrevious(WINDOW*& audioInfoWindow, AudioManager& player, std::atomic<AudioState>& audioState, AppState& appState);

/** @brief Reconstructs the audioFiles container. */
void refreshFiles(WINDOW*& fileWindow, AppState& appState);

/** @brief Used to update the UI */
void redrawScreen(WINDOW*& fileWindow, WINDOW*& audioInfoWindow, AppState& appState);