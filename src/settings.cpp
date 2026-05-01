#include <fstream>
#include <string>

#include "states.hpp"
#include "settings.hpp"

void saveSettings(AppState& appState) {
    std::ofstream settingsFile(appState.audioppSettingsFile, std::ios::trunc);

    if (settingsFile.is_open()) {

        settingsFile << "VOLUME=" << appState.audioDisplayState.volume << '\n';
        settingsFile << "REPEAT_MODE=" << static_cast<int>(appState.repeatMode) << '\n';
        settingsFile << "SORT=" << appState.sortActions.sortType << '\n';
        settingsFile << "SORT_REVERSED=" << appState.sortActions.reversed << '\n';

        settingsFile.close();
    }
}

void loadSettings(AppState& appState) {
    std::ifstream settingsFile(appState.audioppSettingsFile);
    std::string line;

    if (settingsFile.is_open()) {

        while (getline(settingsFile, line)) {
            size_t eqSep = line.find('=');

            if (eqSep != std::string::npos) {
                std::string key = line.substr(0, eqSep);
                std::string val = line.substr(eqSep+1);

                if (key == "VOLUME") {

                    try {
                        appState.audioDisplayState.volume = std::stof(val);
                    }
                    catch (...) {
                        appState.audioDisplayState.volume = 0.7f;
                    }
                }

                if (key == "REPEAT_MODE") {
                    try {
                        appState.repeatMode = static_cast<RepeatModes>(std::stoi(val));
                    }
                    catch (...) {
                        appState.repeatMode = static_cast<RepeatModes>(0);
                    }
                }

                if (key == "SORT") {
                    try {
                        appState.sortActions.sortType = val;
                    }
                    catch (...) {
                        appState.sortActions.sortType = "name";
                    }
                }

                if (key == "SORT_REVERSED") {
                    try {
                        appState.sortActions.reversed = std::stoi(val);
                    }
                    catch (...) {
                        appState.sortActions.reversed = false;
                    }
                }
            }
        }
        settingsFile.close();
    }
    else { // Fallback incase the file couldn't open
        appState.audioDisplayState.volume = 0.7f;
        appState.repeatMode = static_cast<RepeatModes>(0);
        appState.sortActions.sortType = "name";
        appState.sortActions.reversed = false;
    }

}
