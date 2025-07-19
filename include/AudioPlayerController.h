#ifndef AUDIO_PLAYER_CONTROLLER_H
#define AUDIO_PLAYER_CONTROLLER_H

#include <Arduino.h>
#include "AudioPlayer.h"
#include "BluetoothAudioSource.h"
#include "WebServerManager.h"
#include "WiFiManager.h"

class AudioPlayerController {
private:
    AudioPlayer audioPlayer;
    BluetoothAudioSource bluetoothSource;
    WebServerManager webServer;
    WiFiManager wifiManager;
    
    // Configuration
    String wifiSSID;
    String wifiPassword;
    String bluetoothDeviceName;
    String audioFileName;
    
    // Status tracking
    unsigned long lastStatusUpdate;
    unsigned long statusUpdateInterval;
    bool systemReady;
    
    void printSystemStatus();
    void printStartupInfo();
    
public:
    AudioPlayerController();
    ~AudioPlayerController();
    
    // Configuration methods
    void setWiFiCredentials(const char* ssid, const char* password);
    void setBluetoothDeviceName(const char* name);
    void setAudioFile(const char* filename);
    void setStatusUpdateInterval(unsigned long interval);
    
    // System lifecycle
    bool begin();
    void update();
    
    // Status getters
    bool isSystemReady() const { return systemReady; }
    AudioState getAudioState() const { return audioPlayer.getState(); }
    bool isBluetoothConnected() { return bluetoothSource.isConnected(); }
    bool isWiFiConnected() const { return wifiManager.isConnected(); }
    
    // Manual control
    bool playAudio();
    void stopAudio();
    
    // Volume control (0-100 scale)
    void setVolume(uint8_t percentage);  // 0-100
    uint8_t getVolume() const;           // Returns 0-100
    
    // Volume mapping utilities
    static uint8_t mapVolumeToPercentage(uint8_t value127);   // 0-127 -> 0-100
    static uint8_t mapPercentageToVolume(uint8_t percentage); // 0-100 -> 0-127
};

#endif 