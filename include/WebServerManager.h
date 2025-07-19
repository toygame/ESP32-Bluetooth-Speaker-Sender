#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "AudioPlayer.h"
#include "WiFiManager.h"

// Forward declaration to avoid circular dependency
class AudioPlayerController;

class WebServerManager {
private:
    AsyncWebServer server;
    AudioPlayer* audioPlayer;
    WiFiManager* wifiManager;
    AudioPlayerController* controller;
    uint16_t port;
    bool started;
    
    void setupRoutes();
    void handlePlayRequest(AsyncWebServerRequest *request);
    void handleStopRequest(AsyncWebServerRequest *request);
    void handleStatusRequest(AsyncWebServerRequest *request);
    void handleRootRequest(AsyncWebServerRequest *request);
    void handleVolumeGetRequest(AsyncWebServerRequest *request);
    void handleVolumeSetRequest(AsyncWebServerRequest *request);
    
public:
    WebServerManager(uint16_t port = 80);
    ~WebServerManager();
    
    bool begin();
    void setAudioPlayer(AudioPlayer* player);
    void setWiFiManager(WiFiManager* wifi);
    void setController(AudioPlayerController* ctrl);
    
    bool isStarted() const { return started; }
    uint16_t getPort() const { return port; }
    
    String getBaseURL() const;
};

#endif 