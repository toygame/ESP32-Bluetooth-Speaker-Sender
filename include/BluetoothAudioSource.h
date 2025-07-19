#ifndef BLUETOOTH_AUDIO_SOURCE_H
#define BLUETOOTH_AUDIO_SOURCE_H

#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "AudioPlayer.h"

class BluetoothAudioSource {
private:
    BluetoothA2DPSource a2dp_source;
    AudioPlayer* audioPlayer;
    String deviceName;
    uint8_t volume;
    
    static BluetoothAudioSource* instance;
    static int32_t audioCallback(Frame* data, int32_t frameCount);
    
public:
    BluetoothAudioSource();
    ~BluetoothAudioSource();
    
    bool begin(const char* deviceName);
    void setAudioPlayer(AudioPlayer* player);
    void setVolume(uint8_t vol);
    void setAutoReconnect(bool enable);
    
    bool isConnected();
    const char* getDeviceName() const { return deviceName.c_str(); }
    uint8_t getVolume() const { return volume; }
};

#endif 