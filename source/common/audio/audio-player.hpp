#pragma once

#include <string>

#include <miniaudio.h>

namespace our {

class AudioPlayer {
    ma_engine engine{};
    ma_sound sound{};
    bool engineInitialized = false;
    bool soundInitialized = false;

public:
    AudioPlayer() = default;
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    bool play(const std::string& filepath, float volume = 1.0f);
    bool playLoop(const std::string& filepath, float volume = 1.0f);
    void stop();
};

}
