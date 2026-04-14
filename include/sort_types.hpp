#pragma once

#include <vector>
#include <filesystem>

#include "states.hpp"

namespace fs = std::filesystem;

void sortByName(AppState& appState);

void sortByLastWrite(AppState& appState);

void sortBySize(AppState& appState);

void sortByExtension(AppState& appState);