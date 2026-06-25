#include "audio.h"
#include <string>

const std::string ASSET_PREFIX = "../assets/";

AudioManager::AudioManager() {}

void AudioManager::init() {
    moveSound = loadSound((ASSET_PREFIX + "move.wav").c_str());
    captureSound = loadSound((ASSET_PREFIX + "capture.wav").c_str());
    quantumEnterSound = loadSound((ASSET_PREFIX + "quantum_enter.wav").c_str());
    quantumCollapseSound = loadSound((ASSET_PREFIX + "quantum_collapse.wav").c_str());
    checkSound = loadSound((ASSET_PREFIX + "check.wav").c_str());
    checkmateSound = loadSound((ASSET_PREFIX + "checkmate.wav").c_str());
    missSound = loadSound((ASSET_PREFIX + "miss.wav").c_str());
    buttonClickSound = loadSound((ASSET_PREFIX + "button_click.wav").c_str());
    moveInvalidSound = loadSound((ASSET_PREFIX + "move_invalid.wav").c_str());
}

AudioManager::~AudioManager() {
    UnloadSound(moveSound);
    UnloadSound(captureSound);
    UnloadSound(quantumEnterSound);
    UnloadSound(quantumCollapseSound);
    UnloadSound(checkSound);
    UnloadSound(checkmateSound);
    UnloadSound(missSound);
    UnloadSound(buttonClickSound);
    UnloadSound(moveInvalidSound);
}

Sound AudioManager::loadSound(const char* path) {
    Sound s = LoadSound(path);
    return s;
}

void AudioManager::playMove()           { PlaySound(moveSound); }
void AudioManager::playCapture()        { PlaySound(captureSound); }
void AudioManager::playQuantumEnter()   { PlaySound(quantumEnterSound); }
void AudioManager::playQuantumCollapse(){ PlaySound(quantumCollapseSound); }
void AudioManager::playCheck()          { PlaySound(checkSound); }
void AudioManager::playCheckmate()      { PlaySound(checkmateSound); }
void AudioManager::playMiss()           { PlaySound(missSound); }
void AudioManager::playButtonClick()    { PlaySound(buttonClickSound); }
void AudioManager::playMoveInvalid()    { PlaySound(moveInvalidSound); }

void AudioManager::setVolume(float vol) {
    volume = vol;
    SetSoundVolume(moveSound, volume);
    SetSoundVolume(captureSound, volume);
    SetSoundVolume(quantumEnterSound, volume);
    SetSoundVolume(quantumCollapseSound, volume);
    SetSoundVolume(checkSound, volume);
    SetSoundVolume(checkmateSound, volume);
    SetSoundVolume(missSound, volume);
    SetSoundVolume(buttonClickSound, volume);
    SetSoundVolume(moveInvalidSound, volume);
}
