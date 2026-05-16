#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <filesystem>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include "audiomanager.hpp"
#include "states.hpp"
#include "file_commands.hpp"
#include "animations.hpp"

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

namespace fs = std::filesystem;

namespace {
    // Resets UI-related playback state when audio stops.
    void resetUIRelatedStates(AppState* appState) {
        appState->playingIndex = -1;
        appState->audioInfoState.audioName = "";
    }
} // namespace

AudioManager::AudioManager(AudioInfoState* audInfoState, AppState* aState, std::atomic<AudioState>* audioAtomic, WINDOW** audioVisWin) {
    audioState = audioAtomic;
    appState = aState;
    audioInfoState = audInfoState;
    audioVisualWindow = audioVisWin;

    volumeSlider.store(audioInfoState->volume);
}

// Used to send a stop signal to an existing thread and waits for it to clean up
void AudioManager::terminateAudioThread() {
    if (!audioThreadActive) return; // if there's no active thread, then we simply return

    audioInfoState->samplesReady.store(false);
    audioInfoState->visThreadShouldExit.store(true);

    if (visThread.joinable()) visThread.join();

    audioState->store(AudioState::STOPPED);
    if (audioThread.joinable()) audioThread.join();
    audioThreadActive = false;
}

/*
* Plays audio and sets up the audio thread. It signals a stop first, checks if we can join the thread
* so that if there's an active thread that thread exits the loop, resets states
* then the new thread comes in and plays the new audio.
*/
void AudioManager::triggerAudioThread(char* filePath) {

    // Wait for the existing thread to finish its cleanup before starting a new one
    terminateAudioThread();

    audioThreadActive = false;
    audioFinished = false;

    // Validate that the file does exist
	fs::path pathObj = fs::u8path(filePath);

    if (!fs::exists(pathObj)) {
        printError("File not found!", *appState);

        std::string fileName = appState->vfs.audioFileNames[appState->playingIndex];
        resetUIRelatedStates(appState);
        rm({fileName}, {}, *appState);

        return;
    }

    audioFile = filePath;

    audioThread = std::thread(&AudioManager::manageAudioThread, this);
    audioThreadActive = true;

    audioInfoState->samplesReady.store(false);
    audioInfoState->bufWriteIdx = 0;
    audioInfoState->visThreadShouldExit.store(false);
    visThread = std::thread(renderWaveform,
    std::ref(*audioVisualWindow),
    std::ref(*appState),
    std::ref(*audioState)
    );
}

std::string AudioManager::getFullAudioDuration() {
    // Convert total seconds into an MM:SS format string
    int minutes{static_cast<int>(totalSeconds) / 60};
    remainingSeconds = totalSeconds - (minutes * 60);

    std::stringstream ss;
    ss << minutes << ":" << std::setfill('0') << std::setw(2) << static_cast<int>(remainingSeconds);
    return ss.str();
}

void AudioManager::formatElapsed() {
    // Break down total elapsed time into minutes and seconds for the UI
    int elapsedMinutes = static_cast<int>(totalElapsedTime) / 60;
    int elapsedSeconds = static_cast<int>(totalElapsedTime) % 60;

    audioInfoState->elapsedMinutes = elapsedMinutes;
    audioInfoState->elapsedSeconds = elapsedSeconds;
}

// Initializes miniaudio. After initializing miniaudio, it will set the state to playing.
AudioState AudioManager::initializeMA() {
	fs::path pathObj = fs::u8path(audioFile);
    decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    decoderConfig.seekPointCount = 128; // Helps with smoother seeking especially with mp3s

    #ifdef _WIN32
        ma_result decoderInitRes = ma_decoder_init_file_w(pathObj.c_str(), &decoderConfig, &decoder);
    #else
        ma_result decoderInitRes = ma_decoder_init_file(pathObj.c_str(), &decoderConfig, &decoder);
    #endif

    if (decoderInitRes != MA_SUCCESS) {
        audioState->store(AudioState::STOPPED);
        return AudioState::FAILED;
    }

    // Reset playback position for new file
    frameCursor = 0;
    totalFrames = 0;

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = this;


    ma_result deviceInitRes = ma_device_init(NULL, &deviceConfig, &device);

    if (deviceInitRes != MA_SUCCESS) {
        audioState->store(AudioState::STOPPED);
        ma_decoder_uninit(&decoder);
        return AudioState::FAILED;
    }

    ma_result deviceStartRes = ma_device_start(&device);

    if (deviceStartRes != MA_SUCCESS) {
        audioState->store(AudioState::STOPPED);
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return AudioState::FAILED;
    }

    // Retrieve file length in frames and calculate total duration in seconds
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);

    // --- Other ---
    totalSeconds = static_cast<double>(totalFrames) / deviceConfig.sampleRate;

    totalElapsedTime = 0;

    frameOffset = 5 * deviceConfig.sampleRate; // 5 seconds in frames

    wasPaused = false;
    audioState->store(AudioState::PLAYING);

    // Set initial seek time to the past to avoid triggering double-press on first load
    lastBackSeekTime = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    return AudioState::SUCCESS;
}

// Miniaudio's thread for playback
void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioManager* pManager{static_cast<AudioManager*>(pDevice->pUserData)};
    AudioState currentState = pManager->audioState->load();

    if (currentState == AudioState::PAUSED) {
        /* Clearing the buffer here for miniaudio to continue reading data but without playing the actual audio.
         * Reason for this approach for pausing is just to allow seeking while paused.
         * Using ma_device_stop() stops data_callback() which ends up blocking seeking while the audio is paused. */
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame);
        return;
    }

    // If total frames were to be zero, then that means the file hasn't loaded yet
    if (currentState == AudioState::SEEKING_FWD && pManager->totalFrames != 0) {
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame); // Clearing the buffer to avoid a pop sound

        ma_uint64 newPos = pManager->frameCursor + pManager->frameOffset;

        // If seeking forward goes beyond the track length, trigger the next song
        if (newPos >= pManager->totalFrames) {
            pManager->appState->shouldPlayNext = true;
            pManager->audioState->store(AudioState::STOPPED);
            return;
        }

        ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);
        pManager->audioState->store(pManager->wasPaused ? AudioState::PAUSED : AudioState::PLAYING);
        pManager->wasPaused = false;

        return;
    }

    // If total frames were to be zero, then that means the file hasn't loaded yet
    if (currentState == AudioState::SEEKING_BWD && pManager->totalFrames != 0) {
        ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        memset(pOutput, 0, frameCount * bytesPerFrame); // Clearing the buffer to avoid a pop sound

        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastClick =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - pManager->lastBackSeekTime);

        // Update timestamp immediately so the next callback can calculate the double-press window
        pManager->lastBackSeekTime = now;

        const auto DOUBLE_PRESS_WINDOW = std::chrono::milliseconds(500);

        // if we are in the first five seconds of the audio
        if (pManager->frameCursor <= pManager->frameOffset) {

            // If double-pressed near start, signal a previous track change and stop current thread
            if (timeSinceLastClick < DOUBLE_PRESS_WINDOW) {
                pManager->appState->shouldPlayPrev = true;
                pManager->audioState->store(AudioState::STOPPED); // Signal loop to exit
                return;
            }
            // Otherwise it's a single press within the first 5 seconds, so we just reset the frames to 0
            pManager->frameCursor = 0;
            ma_decoder_seek_to_pcm_frame(&pManager->decoder, 0);
        }
        else {
            // If further in the song, just perform a 5second seek back
            ma_uint64 newPos = pManager->frameCursor - pManager->frameOffset;
            ma_decoder_seek_to_pcm_frame(&pManager->decoder, newPos);
        }

        // Return to the previous playback state to stop the seek loop
        pManager->audioState->store(pManager->wasPaused ? AudioState::PAUSED : AudioState::PLAYING);
        pManager->wasPaused = false;

        return;
    }

    if (pManager->audioState->load() != AudioState::STOPPED) {
        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(&pManager->decoder, pOutput, frameCount, &framesRead);

        float* samples = static_cast<float*>(pOutput);

        // Collect the samples for the visual
        if (!pManager->audioInfoState->samplesReady.load()) {
            int samplesToRead = framesRead * 2;

            for (int i = 0; i < samplesToRead; i += 2) {
                pManager->audioInfoState->samplesBuf[pManager->audioInfoState->bufWriteIdx] = samples[i];
                pManager->audioInfoState->bufWriteIdx++;

                if (pManager->audioInfoState->bufWriteIdx >= pManager->audioInfoState->samplesBuf.size()) {
                    pManager->audioInfoState->bufWriteIdx = 0;
                    pManager->audioInfoState->samplesReady.store(true);
                }
            }
        }

        // Used for volume
            float gain = std::pow(pManager->volumeSlider.load(std::memory_order_relaxed), 3);

            ma_uint32 sampleCount = frameCount * pDevice->playback.channels;
            for (int i = 0; i < sampleCount; i++) {
                samples[i] *= gain;
            }

        // If the decoder provides fewer frames than requested, it means we hit the end of the file
        if (framesRead < frameCount) {

            if (pManager->appState->repeatMode == RepeatModes::REPEAT_ONE) {
                pManager->frameCursor = 0;
                ma_decoder_seek_to_pcm_frame(&pManager->decoder, 0);
            }
            else {
                pManager->audioFinished = true;
                pManager->audioState->store(AudioState::STOPPED);
            }

        }
    }
}

void AudioManager::manageAudioThread() {
    if (initializeMA() == AudioState::FAILED) {
        audioState->store(AudioState::FAILED);
        resetUIRelatedStates(appState);
        return;
    }

    // UI related
    audioInfoState->audioName = appState->vfs.audioFileNames[appState->playingIndex];
    audioInfoState->duration = getFullAudioDuration();
    audioInfoState->shouldDrawAudioInfo = true;

    audioInfoState->volume = volumeSlider;
    audioInfoState->shouldUpdateVolOrRepeatTxt = true;

    appState->shouldRedrawScreen = true;

    //------------
    while (audioState->load() != AudioState::STOPPED) {
        // Sync the current frame cursor with the decoder position
        ma_decoder_get_cursor_in_pcm_frames(&decoder, &frameCursor);

        totalElapsedTime = static_cast<double>(frameCursor) / deviceConfig.sampleRate;

        // Handling the visualizer by making a smooth fade in/fade out depending on state
        if (audioState->load() == AudioState::PLAYING) {
            ma_device_start(&device);
        }
        else if (audioState->load() == AudioState::PAUSED) {
            audioInfoState->samplesBuf.fill(0);
            audioInfoState->samplesReady.store(true);
        }

        // Update display state (skip during resize to avoid flickering/potential crashes)
        if (!appState->shouldResize) {
            formatElapsed();
            audioInfoState->totalSeconds = totalSeconds;
            audioInfoState->totalElapsedTime = totalElapsedTime;
            audioInfoState->shouldRenderAnimation = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }

    if (appState->repeatMode == RepeatModes::REPEAT_OFF &&
        appState->playingIndex == appState->numberOfFiles-1 && audioFinished) {

        resetUIRelatedStates(appState);
        audioInfoState->shouldCleanup = true;
        appState->shouldRedrawScreen = true;
    }
    else if (audioFinished) appState->shouldPlayNext = true;

    uninitializeMA();
}

void AudioManager::uninitializeMA() {
    ma_device_stop(&device);
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
}

void AudioManager::playNext(std::atomic<AudioState>& audioState, AppState& appState) {
    // Increment index with wrapping to the start of the list
    appState.playingIndex = (appState.playingIndex + 1) % appState.numberOfFiles;

    char* audioFilePath = const_cast<char*>(appState.vfs.audioMap[appState.vfs.audioFileNames[appState.playingIndex]].c_str());
    this->triggerAudioThread(audioFilePath);

    appState.shouldPlayNext = false;
}

void AudioManager::playPrevious(std::atomic<AudioState> &audioState, AppState &appState) {
    // Decrement index with wrapping to the end of the list
    appState.playingIndex = (appState.playingIndex - 1 + appState.numberOfFiles) % appState.numberOfFiles;

    char* audioFilePath = const_cast<char*>(appState.vfs.audioMap[appState.vfs.audioFileNames[appState.playingIndex]].c_str());
    this->triggerAudioThread(audioFilePath);

    appState.shouldPlayPrev = false;
}

AudioManager::~AudioManager() {
    terminateAudioThread();
}