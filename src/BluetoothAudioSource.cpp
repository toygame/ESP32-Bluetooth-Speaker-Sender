#include "BluetoothAudioSource.h"

// Static member initialization
BluetoothAudioSource* BluetoothAudioSource::instance = nullptr;

BluetoothAudioSource::BluetoothAudioSource() 
    : audioPlayer(nullptr), volume(30) {
    instance = this;
}

BluetoothAudioSource::~BluetoothAudioSource() {
    instance = nullptr;
}

bool BluetoothAudioSource::begin(const char* deviceName) {
    Serial.println("Initializing Bluetooth A2DP Source...");
    
    this->deviceName = deviceName;
    
    // Set up Bluetooth A2DP Source
    a2dp_source.set_auto_reconnect(true);
    a2dp_source.set_data_callback_in_frames(audioCallback);
    a2dp_source.set_volume(volume);

    Serial.printf("Starting Bluetooth A2DP Source with name: %s\n", deviceName);
    a2dp_source.start(deviceName);
    
    Serial.printf("Please pair your Bluetooth speaker with '%s'\n", deviceName);
    return true;
}

void BluetoothAudioSource::setAudioPlayer(AudioPlayer* player) {
    this->audioPlayer = player;
}

void BluetoothAudioSource::setVolume(uint8_t vol) {
    this->volume = vol;
    a2dp_source.set_volume(vol);
}

void BluetoothAudioSource::setAutoReconnect(bool enable) {
    a2dp_source.set_auto_reconnect(enable);
}

bool BluetoothAudioSource::isConnected() {
    return a2dp_source.is_connected();
}

int32_t BluetoothAudioSource::audioCallback(Frame* data, int32_t frameCount) {
    if (instance && instance->audioPlayer) {
        // Convert Frame to AudioFrame and call AudioPlayer
        AudioFrame* audioData = reinterpret_cast<AudioFrame*>(data);
        return instance->audioPlayer->getAudioData(audioData, frameCount);
    }
    
    // If no audio player, send silence
    for (int i = 0; i < frameCount; i++) {
        data[i].channel1 = 0;
        data[i].channel2 = 0;
    }
    return frameCount;
} 