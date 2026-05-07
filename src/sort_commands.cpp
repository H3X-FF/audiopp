#include <vector>
#include <algorithm>
#include <filesystem>
#include <string>
#include <cctype>

#include "states.hpp"
#include "sort_commands.hpp"

namespace fs = std::filesystem;

namespace {
    bool sortByNameComparator(const fs::path& a, const fs::path& b, AppState& appState) {
        std::string s1 = a.filename().string();
        std::string s2 = b.filename().string();

        return std::lexicographical_compare(
            s1.begin(), s1.end(),
            s2.begin(), s2.end(),
            [&](unsigned char c1, unsigned char c2) {
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    }

    bool sortByLastWriteComparator(const fs::path& a, const fs::path& b, AppState& appState) {
        auto it_a = appState.vfs.audioMap.find(a.string());
        auto it_b = appState.vfs.audioMap.find(b.string());

        if (it_a == appState.vfs.audioMap.end() || it_b == appState.vfs.audioMap.end()) {
            return a.string() < b.string();
        }
        auto p1 = fs::last_write_time(it_a->second);
        auto p2 = fs::last_write_time(it_b->second);

        return p1 > p2;
    }

    bool sortBySizeComparator(const fs::path& a, const fs::path& b, AppState& appState) {
        auto it_a = appState.vfs.audioMap.find(a.string());
        auto it_b = appState.vfs.audioMap.find(b.string());

        if (it_a == appState.vfs.audioMap.end() || it_b == appState.vfs.audioMap.end()) {
            return a.string() < b.string();
        }
        auto p1 = fs::file_size(it_a->second);
        auto p2 = fs::file_size(it_b->second);

        return p1 > p2;
    }



    bool sortByExtensionComparator(const fs::path& a, const fs::path& b, AppState& appState) {
        std::string s1 = a.filename().extension().string();
        std::string s2 = b.filename().extension().string();

        return std::lexicographical_compare(
            s1.begin(), s1.end(),
            s2.begin(), s2.end(),
            [&](unsigned char c1, unsigned char c2) {
                return std::tolower(c1) < std::tolower(c2);
            }
        );
    }
}

void sortByName(AppState& appState) {

    if (appState.sortActions.reversed) {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
        return sortByNameComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
            return sortByNameComparator(a, b, appState);
        });
    }

}

void sortByLastWrite(AppState& appState) {
    if (appState.sortActions.reversed) {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
            return sortByLastWriteComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
        return sortByLastWriteComparator(a, b, appState);
        });
    }
}

void sortBySize(AppState& appState) {
    if (appState.sortActions.reversed) {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
            return sortBySizeComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
        return sortBySizeComparator(a, b, appState);
        });
    }
}

void sortByExtension(AppState& appState) {
    if (appState.sortActions.reversed) {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
            return sortByExtensionComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.vfs.audioFileNames.begin(), appState.vfs.audioFileNames.end(), [&](const fs::path& a, const fs::path& b) {
        return sortByExtensionComparator(a, b, appState);
        });
    }
}

void sortList(const std::vector<std::string>& args, const std::vector<std::string>& flags, AppState& appState) {

    if (args[0] != "name" && args[0] != "lwt" && args[0] != "size" &&  args[0] != "ext") {
        printError("Invalid argument for sort", appState);
        return;
    }

    if (flags.size() > 1) {
        printError("Too many flags!", appState);
        return;
    }

    appState.sortActions.sortType = args[0];
    appState.sortActions.reversed = (!flags.empty() && flags[0] == "--reverse");

    // Sorting will be handled when the list is refreshed
    appState.shouldRefreshFiles = true;
}
