#pragma once

#include <vector>
#include <string>

#include "states.hpp"

/** @brief 'scan' command to find and copy or move audio files to the program's audio directory. */
void scan(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState);
void mv(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState);
void rm(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState);
void rname(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState);