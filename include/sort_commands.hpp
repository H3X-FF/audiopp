#pragma once
#ifndef AUDIOPP_SORT_COMMANDS_HPP
#define AUDIOPP_SORT_COMMANDS_HPP

#include <vector>

#include "states.hpp"

void sortByName(AppState& appState);

void sortByLastWrite(AppState& appState);

void sortBySize(AppState& appState);

void sortByExtension(AppState& appState);

void sortList(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState);

#endif
