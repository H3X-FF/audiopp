#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "runcommand.hpp"
#include "ncursesw/curses.h"

namespace fs = std::filesystem;

void moveFiles(const fs::directory_entry& entry, const fs::path& audioPath, std::string operation) {
    std::string fileExtension{entry.path().extension()};

    // Case-insensitive extension check
    std::transform(fileExtension.begin(), fileExtension.end(),
        fileExtension.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    if (fileExtension == ".wav") {
        fs::copy(entry.path(), audioPath);
        if (operation == "--move") fs::remove(entry.path());
    }
}

void scan(const std::vector<std::string>& args, const std::vector<std::string>& flags) {
    fs::path pathToAudioFiles{args[0]};
    bool isRecursive{false};
    std::string operation{""};

    // Validate source directory
    if (!fs::is_directory(pathToAudioFiles)) {
        move(LINES-1, 0);
        clrtoeol();
        attron(COLOR_PAIR(3));
        printw("Not a directory: %s", pathToAudioFiles.c_str());
        attroff(COLOR_PAIR(3));
        return;
    }

    // Parse command flags
    for (int i{0}; i < flags.size(); i++) {
        if (flags[i] == "--move" || flags[i] =="--copy") operation = flags[i];
        else isRecursive = true;
    }

    fs::path audioPath{APP_PATH};
    audioPath /= "audio";

    // Non-recursive file scanning and importation
    if (!isRecursive) {
        for (const auto& entry : fs::directory_iterator(pathToAudioFiles)) {
            moveFiles(entry, audioPath, operation);
        }
    }
    // Recursive file scanning and importation
    else {
        for (const auto& entry : fs::recursive_directory_iterator(pathToAudioFiles)) {
            moveFiles(entry, audioPath, operation);
        }
    }
}