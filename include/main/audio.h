#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

class AudioManager {
    Sound moveSound;
    Sound captureSound;
    Sound quantumEnterSound;
    Sound quantumCollapseSound;
    Sound checkSound;
    Sound checkmateSound;
    Sound missSound;
    Sound buttonClickSound;
    Sound moveInvalidSound;
    float volume = 1.0f;

    Sound loadSound(const char* path);

public:
    AudioManager();
    ~AudioManager();

    void playMove();
    void playCapture();
    void playQuantumEnter();
    void playQuantumCollapse();
    void playCheck();
    void playCheckmate();
    void playMiss();
    void playButtonClick();
    void playMoveInvalid();

    void setVolume(float vol);
};

#endif
