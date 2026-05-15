#include <cmath>
#include <kissfft/kiss_fft.h>
#include <kissfft/kiss_fftr.h>
#include <algorithm>

#include "animations.hpp"

#include <bits/this_thread_sleep.h>

#include "states.hpp"
#include "ui.hpp"

#ifdef _WIN32
    #include <PDCursesMod/curses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

void renderWaveform(WINDOW*& audioVisualWindow, AppState& appState, std::atomic<AudioState>& audioState) {
    const int SAMPLE_SIZE = 1024;
    kiss_fftr_cfg fftConfig = kiss_fftr_alloc(SAMPLE_SIZE, 0, nullptr, nullptr);
    std::array<float, SAMPLE_SIZE> previousFrame;
    previousFrame.fill(0.0f);

    while (audioState.load() != AudioState::STOPPED) {

        if (appState.audioInfoState.samplesReady.load() && !appState.shouldResize) {
            int winHeight, winWidth;
            getmaxyx(audioVisualWindow, winHeight, winWidth);

            // Set the spectrum floor to the bottom of the window (above the border)
            int spectrumFloorY = winHeight - 2;
            int maxHeight = winHeight - 2;

            // Hann window to keep the bars clean
            std::array<float, 1024> windowedSamples;
            for (int i = 0; i < 1024; i++) {
                float hann = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / 1023.0f));
                windowedSamples[i] = appState.audioInfoState.samplesBuf[i] * hann;
            }


            std::array<kiss_fft_cpx, 513> fftOutput;
            kiss_fftr(fftConfig, windowedSamples.data(), fftOutput.data());

            // Extracts frequency magnitudes
            std::array<float, 513> fftMagnitudes;
            float globalPeak = 0.001f;
            for (int b = 0; b < 513; b++) {
                fftMagnitudes[b] = std::sqrt(fftOutput[b].r * fftOutput[b].r + fftOutput[b].i * fftOutput[b].i);
                if (fftMagnitudes[b] > globalPeak) {
                    globalPeak = fftMagnitudes[b];
                }
            }

            // maps to screen columns (Using only the audible ~250 bins)
            wattron(audioVisualWindow, COLOR_PAIR(4));
            for (int x = 0; x < winWidth; x++) {
                float progress = static_cast<float>(x) / winWidth;

                // logarithmic mapping
                // Stretches bass/mids across the window and leaves the high treble on the right
                int startBin = static_cast<int>(std::pow(progress, 2.0f) * 190);
                int endBin = static_cast<int>(std::pow(static_cast<float>(x + 1) / winWidth, 2.0f) * 190);

                if (endBin <= startBin) endBin = startBin + 1;

                float localPeak = 0.0f;
                for (int b = startBin; b < endBin && b < 513; b++) {
                    if (fftMagnitudes[b] > localPeak) {
                        localPeak = fftMagnitudes[b];
                    }
                }

                // Treble balance
                float trebleBoost = 1.0f + (progress * 4.5f);
                int targetHeight = static_cast<int>((localPeak / globalPeak) * maxHeight * trebleBoost);
                if (targetHeight > maxHeight) targetHeight = maxHeight;

                // Smoothed fall dynamics
                float diff = targetHeight - previousFrame[x];
                float smoothHeight = (diff > 0) ? (previousFrame[x] + diff * 0.70f) : (previousFrame[x] * 0.84f);
                previousFrame[x] = smoothHeight;

                int barHeight = static_cast<int>(smoothHeight);

                for (int y = 0; y < winHeight; y++) {
                    mvwaddwstr(audioVisualWindow, y, x, L" ");
                }

                // Render columns upward from the spectrum floor
                for (int y = 0; y <= barHeight; y++) {
                    mvwaddwstr(audioVisualWindow, spectrumFloorY - y, x, L"▌");
                }
            }
            wattroff(audioVisualWindow, COLOR_PAIR(4));


            createBorder(audioVisualWindow);
            wnoutrefresh(audioVisualWindow);
            appState.audioInfoState.samplesReady.store(false);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    werase(audioVisualWindow);
    createBorder(audioVisualWindow);
    wnoutrefresh(audioVisualWindow);

    kiss_fftr_free(fftConfig);
}

void renderProgress(WINDOW*& audioInfoWindow, int windowWidth, AudioInfoState& audioInfoState) {
    int rightSidePadding = 8;
    int leftSidePadding = 3;
    int yPos = 3;
    int barWidth = windowWidth - rightSidePadding;

    double progress = audioInfoState.totalSeconds > 0 ? audioInfoState.totalElapsedTime / audioInfoState.totalSeconds : 0;
    double filled = progress * barWidth;

    int fullBlocks = static_cast<int>(filled);
    double remainder = filled - fullBlocks;

    const wchar_t* partials[7] = {L"▏", L"▎", L"▍", L"▌", L"▋", L"▊", L"▉"};

    wmove(audioInfoWindow, yPos, 0);
    wclrtoeol(audioInfoWindow);

    wattron(audioInfoWindow, COLOR_PAIR(5));
    mvwaddwstr(audioInfoWindow, yPos, leftSidePadding-1, L"▉");
    mvwaddwstr(audioInfoWindow, yPos, barWidth+leftSidePadding, L"▉");
    wattroff(audioInfoWindow, COLOR_PAIR(5));

    wattron(audioInfoWindow, COLOR_PAIR(4));

    for (int i = 0; i < barWidth; i++) {
        if (i < fullBlocks) {
            mvwaddwstr(audioInfoWindow, yPos, i+leftSidePadding, L"█");
        }
        else if (i == fullBlocks && remainder > 0.1) {
            int block = static_cast<int>(remainder * 7);
            if (block > 6) block = 6;
            mvwaddwstr(audioInfoWindow, yPos, i+leftSidePadding, partials[block]);
        }
    }

    wattroff(audioInfoWindow, COLOR_PAIR(4));

    wmove(audioInfoWindow, yPos+1, leftSidePadding-1);
    wclrtoeol(audioInfoWindow);

    wattron(audioInfoWindow, COLOR_PAIR(4));
    wattron(audioInfoWindow, WA_BOLD);

    wprintw(audioInfoWindow, "Time: %d:%02d/%s",
            audioInfoState.elapsedMinutes, audioInfoState.elapsedSeconds, audioInfoState.duration.c_str());

    wattroff(audioInfoWindow, COLOR_PAIR(4));
    wattroff(audioInfoWindow, WA_BOLD);
}
