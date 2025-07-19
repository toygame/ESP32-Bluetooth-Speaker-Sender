#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

enum AudioState {
    STANDBY,
    PLAYING
};

struct AudioFrame {
    int16_t channel1;
    int16_t channel2;
};

class AudioPlayer {
private:
    File audioFile;
    size_t audioFileSize;
    size_t audioPosition;
    size_t audioDataStart;
    size_t audioDataSize;
    bool audioFileOpen;
    AudioState currentState;
    
    // WAV format info
    uint16_t audioChannels;
    uint32_t audioSampleRate;
    uint16_t audioBitsPerSample;
    
    bool parseWAVHeader();
    
public:
    AudioPlayer();
    ~AudioPlayer();
    
    bool begin();
    bool prepareAudioFile(const char* filename);
    bool startPlayback();
    void stopPlayback();
    
    AudioState getState() const { return currentState; }
    bool isPlaying() const { return currentState == PLAYING; }
    
    int32_t getAudioData(AudioFrame* data, int32_t frameCount);
    
    // Progress info
    uint32_t getPlayedBytes() const;
    uint32_t getTotalBytes() const;
    uint8_t getProgressPercentage() const;
};

#endif 