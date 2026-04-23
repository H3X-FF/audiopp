#pragma once
#ifndef AUDIOPP_VFS_HPP
#define AUDIOPP_VFS_HPP

#include "states.hpp"

/** @brief Checks if ~/.local/audiopp/audiopp_paths_to_audio_files.json exists, if not, then create them */
void initializeDirAndFiles(AppState& appState);

/** @brief Scans the paths stored in ~/.local/audiopp/audiopp_paths_to_audio_files.json and stores them
 * in a hashmap and vector container. */
void getAudioFiles(AppState& appState);

#endif // AUDIOPP_VFS_HPP