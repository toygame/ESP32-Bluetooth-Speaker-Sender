#include "WebServerManager.h"
#include "AudioPlayerController.h"

WebServerManager::WebServerManager(uint16_t port) 
    : server(port), audioPlayer(nullptr), wifiManager(nullptr), controller(nullptr),
      port(port), started(false) {
}

WebServerManager::~WebServerManager() {
    // Server cleanup is handled automatically
}

bool WebServerManager::begin() {
    if (!audioPlayer || !wifiManager) {
        Serial.println("WebServerManager: AudioPlayer and WiFiManager must be set before starting");
        return false;
    }
    
    if (!wifiManager->isConnected()) {
        Serial.println("WebServerManager: WiFi must be connected before starting web server");
        return false;
    }
    
    Serial.println("Setting up Web Server...");
    setupRoutes();
    
    server.begin();
    started = true;
    
    Serial.printf("Web Server started on port %d\n", port);
    Serial.printf("Access web interface at: %s\n", getBaseURL().c_str());
    
    return true;
}

void WebServerManager::setAudioPlayer(AudioPlayer* player) {
    this->audioPlayer = player;
}

void WebServerManager::setWiFiManager(WiFiManager* wifi) {
    this->wifiManager = wifi;
}

void WebServerManager::setController(AudioPlayerController* ctrl) {
    this->controller = ctrl;
}

void WebServerManager::setupRoutes() {
    // API endpoint for playing audio
    server.on("/play1", HTTP_GET, [this](AsyncWebServerRequest *request){
        handlePlayRequest(request);
    });
    
    // API endpoint for stopping audio
    server.on("/stop", HTTP_GET, [this](AsyncWebServerRequest *request){
        handleStopRequest(request);
    });
    
    // Status endpoint
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request){
        handleStatusRequest(request);
    });
    
    // Root endpoint
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        handleRootRequest(request);
    });
    
    // Volume endpoints
    server.on("/volume", HTTP_GET, [this](AsyncWebServerRequest *request){
        handleVolumeGetRequest(request);
    });
    
    server.on("/volume", HTTP_POST, [this](AsyncWebServerRequest *request){
        handleVolumeSetRequest(request);
    });
}

void WebServerManager::handlePlayRequest(AsyncWebServerRequest *request) {
    // Check available memory
    if (ESP.getFreeHeap() < 10000) {
        Serial.printf("WARNING: Low memory: %d bytes\n", ESP.getFreeHeap());
        request->send(503, "text/plain", "Low memory - service temporarily unavailable");
        return;
    }
    
    if (!audioPlayer) {
        request->send(500, "text/plain", "Audio player not available");
        return;
    }
    
    // Check if already playing - skip if playing
    if (audioPlayer->getState() == PLAYING) {
        request->send(409, "text/plain", "Audio already playing - request skipped");
        Serial.println("API: Play request skipped - audio already playing");
        return;
    }
    
    // Try to start playback only if not playing
    if (audioPlayer->startPlayback()) {
        request->send(200, "text/plain", "Audio playback started!");
        Serial.printf("API: Audio playback started via /play1 (Free heap: %d)\n", ESP.getFreeHeap());
    } else {
        request->send(500, "text/plain", "Failed to start audio playback");
        Serial.println("API: Failed to start audio playback via /play1");
    }
}

void WebServerManager::handleStopRequest(AsyncWebServerRequest *request) {
    if (!audioPlayer) {
        request->send(500, "text/plain", "Audio player not available");
        return;
    }
    
    // Check if currently playing
    if (audioPlayer->getState() != PLAYING) {
        request->send(409, "text/plain", "Audio not playing - nothing to stop");
        Serial.println("API: Stop request ignored - audio not playing");
        return;
    }
    
    // Stop playback
    audioPlayer->stopPlayback();
    request->send(200, "text/plain", "Audio playback stopped!");
    Serial.printf("API: Audio playback stopped via /stop (Free heap: %d)\n", ESP.getFreeHeap());
}

void WebServerManager::handleStatusRequest(AsyncWebServerRequest *request) {
    String status = "UNKNOWN";
    if (audioPlayer) {
        status = (audioPlayer->getState() == PLAYING) ? "PLAYING" : "STANDBY";
    }
    
    String response = "Status: " + status + "\n";
    response += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    
    if (wifiManager) {
        response += "WiFi IP: " + wifiManager->getLocalIP() + "\n";
        response += "WiFi RSSI: " + String(wifiManager->getRSSI()) + " dBm\n";
    }
    
    if (audioPlayer && audioPlayer->isPlaying()) {
        response += "Progress: " + String(audioPlayer->getProgressPercentage()) + "%\n";
        response += "Played: " + String(audioPlayer->getPlayedBytes()) + " / " + String(audioPlayer->getTotalBytes()) + " bytes\n";
    }
    
    request->send(200, "text/plain", response);
}

void WebServerManager::handleRootRequest(AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<title>ESP32 Audio Player</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }";
    html += ".container { max-width: 500px; margin: 50px auto; background: white; border-radius: 20px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); overflow: hidden; }";
    html += ".header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; text-align: center; }";
    html += ".header h1 { margin: 0; font-size: 28px; font-weight: 300; }";
    html += ".header p { margin: 10px 0 0 0; opacity: 0.9; font-size: 14px; }";
    html += ".content { padding: 30px; }";
    html += ".button-group { text-align: center; margin-bottom: 30px; }";
    html += ".button { display: inline-block; padding: 15px 30px; margin: 8px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; text-decoration: none; border-radius: 25px; font-size: 16px; font-weight: 500; transition: all 0.3s ease; border: none; cursor: pointer; }";
    html += ".button:hover { transform: translateY(-2px); box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3); }";
    html += ".button.play { background: linear-gradient(135deg, #56ab2f 0%, #a8e6cf 100%); }";
    html += ".button.stop { background: linear-gradient(135deg, #e74c3c 0%, #c0392b 100%); }";
    html += ".button.status { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); }";
    html += ".button.refresh { background: linear-gradient(135deg, #ff9a56 0%, #ffad56 100%); }";
    html += ".status-panel { background: #f8f9fa; padding: 20px; border-radius: 15px; border-left: 4px solid #667eea; }";
    html += ".status-panel h3 { margin: 0 0 15px 0; color: #333; font-size: 18px; }";
    html += ".status-item { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #eee; }";
    html += ".status-item:last-child { border-bottom: none; }";
    html += ".status-label { font-weight: 600; color: #555; }";
    html += ".status-value { color: #333; }";
    html += ".playing { color: #28a745; font-weight: bold; }";
    html += ".standby { color: #6c757d; font-weight: bold; }";
    html += "</style></head><body>";
    
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>&#x1F3B5; ESP32 Audio Player</h1>";
    html += "<p>Web-controlled Bluetooth Audio Streaming</p>";
    html += "</div>";
    
    html += "<div class='content'>";
    html += "<div class='button-group'>";
    html += "<a href='/play1' class='button play'>&#x25B6; Play Audio</a>";
    html += "<a href='/stop' class='button stop'>&#x23F9; Stop Audio</a>";
    html += "<a href='/status' class='button status'>&#x1F4CA; Check Status</a>";
    html += "<a href='javascript:location.reload()' class='button refresh'>&#x1F504; Refresh</a>";
    html += "</div>";
    
    html += "<div class='status-panel'>";
    html += "<h3>System Information</h3>";
    
    if (audioPlayer) {
        String statusText = (audioPlayer->getState() == PLAYING) ? "PLAYING" : "STANDBY";
        String statusClass = (audioPlayer->getState() == PLAYING) ? "playing" : "standby";
        String statusIcon = (audioPlayer->getState() == PLAYING) ? "&#x1F3B5;" : "&#x23F8;";
        html += "<div class='status-item'>";
        html += "<span class='status-label'>Audio Status:</span>";
        html += "<span class='status-value " + statusClass + "'>" + statusIcon + " " + statusText + "</span>";
        html += "</div>";
    }
    
    // Show current volume if controller is available
    if (controller) {
        uint8_t currentVolume = controller->getVolume();
        html += "<div class='status-item'>";
        html += "<span class='status-label'>Volume:</span>";
        html += "<span class='status-value'>&#x1F50A; " + String(currentVolume) + "%</span>";
        html += "</div>";
    }
    
    html += "<div class='status-item'>";
    html += "<span class='status-label'>Free Memory:</span>";
    html += "<span class='status-value'>" + String(ESP.getFreeHeap()) + " bytes</span>";
    html += "</div>";
    
    if (wifiManager) {
        html += "<div class='status-item'>";
        html += "<span class='status-label'>IP Address:</span>";
        html += "<span class='status-value'>" + wifiManager->getLocalIP() + "</span>";
        html += "</div>";
    }
    
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    html += "<script>";
    html += "// Manual refresh button added to prevent watchdog timeout";
    html += "// Auto-refresh was causing async_tcp task issues";
    html += "</script>";
    
    html += "</body></html>";
    
    request->send(200, "text/html; charset=UTF-8", html);
}

String WebServerManager::getBaseURL() const {
    if (wifiManager && wifiManager->isConnected()) {
        return "http://" + wifiManager->getLocalIP() + ":" + String(port) + "/";
    }
    return "";
}

void WebServerManager::handleVolumeGetRequest(AsyncWebServerRequest *request) {
    if (!controller) {
        request->send(500, "application/json", "{\"error\":\"Controller not available\"}");
        return;
    }
    
    uint8_t currentVolume = controller->getVolume();
    String response = "{\"volume\":" + String(currentVolume) + ",\"range\":\"0-100\"}";
    request->send(200, "application/json", response);
    
    Serial.printf("API: Volume get request - Current: %d%%\n", currentVolume);
}

void WebServerManager::handleVolumeSetRequest(AsyncWebServerRequest *request) {
    if (!controller) {
        request->send(500, "application/json", "{\"error\":\"Controller not available\"}");
        return;
    }
    
    // Check for volume parameter
    if (!request->hasParam("volume", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing 'volume' parameter (0-100)\"}");
        return;
    }
    
    // Get volume parameter
    String volumeStr = request->getParam("volume", true)->value();
    int volumeInt = volumeStr.toInt();
    
    // Validate range
    if (volumeInt < 0 || volumeInt > 100) {
        request->send(400, "application/json", "{\"error\":\"Volume must be 0-100\"}");
        return;
    }
    
    // Set volume
    uint8_t newVolume = (uint8_t)volumeInt;
    controller->setVolume(newVolume);
    
    String response = "{\"status\":\"success\",\"volume\":" + String(newVolume) + "}";
    request->send(200, "application/json", response);
    
    Serial.printf("API: Volume set to %d%%\n", newVolume);
} 