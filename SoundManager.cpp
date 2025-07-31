#define MA_ENABLE_VORBIS
#define MA_ENABLE_MP3
#include "miniaudio.h"
#include "SoundManager.h"
#include <thread>
#include <iostream>


namespace SS
{
    SoundManager::SoundManager()
        : engine{}, currentSound{} 
    {
    }

    SoundManager::~SoundManager()
    {
        if (hasSound) {
            ma_sound_uninit(&currentSound);
        }
        ma_engine_uninit(&engine);
    }

    int SoundManager::init()
    {
        if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
            return -1;
        }
        return 1;
    }

    void SoundManager::playMusic(const std::string& filePath)
    {
        // Stop and uninit previous sound
        if (hasSound) {
            ma_sound_stop(&currentSound);
            ma_sound_uninit(&currentSound);
            hasSound = false;
        }

        // Delay before playing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Init and play new sound
        if (ma_sound_init_from_file(&engine, filePath.c_str(), 0, NULL, NULL, &currentSound) == MA_SUCCESS) {
            ma_sound_start(&currentSound);
            hasSound = true;
        }
        else {
            std::cerr << "Failed to load sound: " << filePath << "\n";
        }
    }

    void SoundManager::stopMusic()
    {
        if (hasSound) {
            ma_sound_stop(&currentSound);
        }
    }
}