#pragma once
#include <string>

#include "miniaudio.h"

namespace SS
{

  class SoundManager
  {

  private:
      ma_engine engine;
      ma_sound currentSound;

      bool hasSound = false;

  public:
    
    SoundManager();
    ~SoundManager();

    int init();
    void playMusic( const std::string& filePath);
    void stopMusic();
  };

}