#include <vector>
#include <algorithm>
#include <filesystem>
#include <string>
#include <cctype>

#include "states.hpp"
#include "sort_types.hpp"

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
        auto p1 = last_write_time(a);
        auto p2 = last_write_time(b);

        return p1 > p2;
    }

    bool sortBySizeComparator(const fs::path& a, const fs::path& b, AppState& appState) {
        auto p1 = fs::file_size(a);
        auto p2 = fs::file_size(b);

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
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
        return sortByNameComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
            return sortByNameComparator(a, b, appState);
        });
    }

}

void sortByLastWrite(AppState& appState) {
    if (appState.sortActions.reversed) {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
            return sortByLastWriteComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
        return sortByLastWriteComparator(a, b, appState);
        });
    }
}

void sortBySize(AppState& appState) {
    if (appState.sortActions.reversed) {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
            return sortBySizeComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
        return sortBySizeComparator(a, b, appState);
        });
    }
}

void sortByExtension(AppState& appState) {
    if (appState.sortActions.reversed) {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
            return sortByExtensionComparator(b, a, appState);
        });
    }
    else {
        std::sort(appState.audioFiles.begin(), appState.audioFiles.end(), [&](const fs::path& a, const fs::path& b) {
        return sortByExtensionComparator(a, b, appState);
        });
    }
}