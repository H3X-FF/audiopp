#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>

#include <json/single_include/nlohmann/json.hpp>

#include "vfs.hpp"
#include "states.hpp"
#include "sort_commands.hpp"

using nlohmann::json;

void initializeDirAndFiles(AppState& appState) {
    std::ofstream ofs;

    if (!fs::exists(AUDIOPP_PATH)) {
        fs::create_directories(AUDIOPP_PATH);
    }

    if (!fs::exists(AUDIOPP_FILES_JSON)) {
        // Initialize with empty object of objects structure
        ofs.open(AUDIOPP_FILES_JSON, std::ios::trunc);
        ofs << "{\"main\": {}}";
        ofs.close();
    }
}

void getAudioFiles(AppState& appState) {
    appState.vfs.audioFileNames.clear();

    std::fstream ifs(AUDIOPP_FILES_JSON);

    if (ifs.good()) {
        try {
            json j = json::parse(ifs);

            if (j.is_object() && j.contains(appState.vfs.currPlaylist)) {
                const auto& container = j[appState.vfs.currPlaylist];

                for (const auto& [name, path] : container.items()) {

                    if (fs::exists(path)) {
                        appState.vfs.audioMap[name] = path;
                        appState.vfs.audioFileNames.emplace_back(name);
                    }

                }
            }
        }
        catch (json::exception& e) {
            printError("Couldn't load files. try again by pressing \'r\'");
        }
    }

    ifs.close();


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