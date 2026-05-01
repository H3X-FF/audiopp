#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>

#include <json/single_include/nlohmann/json.hpp>

#include "vfs.hpp"
#include "states.hpp"
#include "sort_commands.hpp"

using nlohmann::json;

void getAudioFiles(AppState& appState) {
    appState.vfs.audioFileNames.clear();

    std::fstream ifs(appState.audioppJsonFile);

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
            printError("Couldn't load files. try again by pressing \'r\'\nOr check audiopp_files.json");
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