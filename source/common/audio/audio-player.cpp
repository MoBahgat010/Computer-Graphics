#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "audio-player.hpp"

namespace our {

AudioPlayer::~AudioPlayer() {
    stop();
    if(engineInitialized) {
        ma_engine_uninit(&engine);
        engineInitialized = false;
    }
}

bool AudioPlayer::play(const std::string& filepath, float volume) {
    if(!engineInitialized) {
        if(ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
            return false;
        }
        engineInitialized = true;
    }

    stop();

    if(ma_sound_init_from_file(&engine, filepath.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &sound) != MA_SUCCESS) {
        return false;
    }

    soundInitialized = true;
    ma_sound_set_looping(&sound, MA_FALSE);
    ma_sound_set_volume(&sound, volume);

    if(ma_sound_start(&sound) != MA_SUCCESS) {
        ma_sound_uninit(&sound);
        soundInitialized = false;
        return false;
    }

    return true;
}

bool AudioPlayer::playLoop(const std::string& filepath, float volume) {
    if(!engineInitialized) {
        if(ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
            return false;
        }
        engineInitialized = true;
    }

    stop();

    if(ma_sound_init_from_file(&engine, filepath.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &sound) != MA_SUCCESS) {
        return false;
    }

    soundInitialized = true;
    ma_sound_set_looping(&sound, MA_TRUE);
    ma_sound_set_volume(&sound, volume);

    if(ma_sound_start(&sound) != MA_SUCCESS) {
        ma_sound_uninit(&sound);
        soundInitialized = false;
        return false;
    }

    return true;
}

void AudioPlayer::stop() {
    if(soundInitialized) {
        ma_sound_stop(&sound);
        ma_sound_uninit(&sound);
        soundInitialized = false;
    }
}

}
