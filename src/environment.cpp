#include <filesystem>
#include <fstream>

#include "environment.hpp"

namespace fs = std::filesystem;

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif __APPLE__
    #include <mach-o/dyld.h>
#else
    #include <unistd.h>
#endif

namespace {
    // This is used in case std::getenv() fails
    fs::path getExecutablePath() {

        #ifdef _WIN32
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(NULL, path, MAX_PATH);
            return fs::path(path).parent_path();

        #elif __APPLE__
            char path[1024];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
                return std::filesystem::path(path).parent_path();
            }

            return fs::current_path(); // fallback

        #else
            return fs::read_symlink("/proc/self/exe").parent_path();

        #endif
    }

} // namespace

fs::path getAudioppPath() {

    #ifdef _WIN32
        const char* usrEnv = std::getenv("APPDATA");
    #else
        const char* usrEnv = std::getenv("HOME");
    #endif

    if (usrEnv) {
        fs::path envPath(usrEnv);

        #ifdef _WIN32
                envPath = envPath / "AudioPlusPlus";
                if (!fs::exists(envPath)) fs::create_directories(envPath);

                return fs::path(usrEnv) / "AudioPlusPlus";

        #else
                envPath = envPath / ".config" / "audiopp";
                if (!fs::exists(envPath)) fs::create_directories(envPath);

                return fs::path(usrEnv) / ".config" / "audiopp";

        #endif
    }

        // if the above fails, then use the executable path as a fallback
        return getExecutablePath();
}

fs::path getAudioppJsonFile(fs::path& audioppPath) {
    fs::path audioppJsonFile(audioppPath / "audiopp_files.json");
    std::ofstream ofs;

    if (!fs::exists(audioppJsonFile)) {
        // Initialize with empty object of objects structure
        ofs.open(audioppJsonFile, std::ios::trunc);
        ofs << "{\"main\": {}}";
        ofs.close();
    }

    return audioppJsonFile;
}

fs::path getAudioppSettingsFile(fs::path& audioppPath) {
    fs::path audioppSettingsFile(audioppPath / "audiopp_settings");
    std::ofstream ofs;

    if (!fs::exists(audioppSettingsFile)) {
        ofs.open(audioppSettingsFile, std::ios::trunc);

        ofs << "VOLUME=0.7" << '\n';
        ofs << "REPEAT_MODE=0" << '\n';
        ofs << "SORT=name" << '\n';
        ofs << "SORT_REVERSED=0" << '\n';

        ofs.close();
    }

    return audioppSettingsFile;
}