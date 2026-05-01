#pragma once
#ifndef AUDIOPP_ENVIRONMENT_HPP
#define AUDIOPP_ENVIRONMENT_HPP

#include <filesystem>
namespace fs = std::filesystem;

fs::path getAudioppPath();

fs::path getAudioppJsonFile(fs::path& audioppPath);

fs::path getAudioppSettingsFile(fs::path& audioppPath);

#endif //AUDIOPP_ENVIRONMENT_HPP