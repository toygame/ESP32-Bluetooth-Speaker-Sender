#include "AudioPlayerController.h"
#include "esp_task_wdt.h"

AudioPlayerController::AudioPlayerController() 
    : lastStatusUpdate(0), statusUpdateInterval(10000), systemReady(false),
      bluetoothDeviceName("AL-01"), audioFileName("/play1.wav") {
}

AudioPlayerController::~AudioPlayerController() {
}

void AudioPlayerController::setWiFiCredentials(const char* ssid, const char* password) {
    this->wifiSSID = ssid;
    this->wifiPassword = password;
}

void AudioPlayerController::setBluetoothDeviceName(const char* name) {
    this->bluetoothDeviceName = name;
}

void AudioPlayerController::setAudioFile(const char* filename) {
    this->audioFileName = filename;
}

void AudioPlayerController::setStatusUpdateInterval(unsigned long interval) {
    this->statusUpdateInterval = interval;
}

bool AudioPlayerController::begin() {
    Serial.begin(115200);
    Serial.println("ESP32 Bluetooth Speaker Sender Starting...");
    Serial.printf("Free heap at startup: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize audio player
    if (!audioPlayer.begin()) {
        Serial.println("Failed to initialize AudioPlayer!");
        return false;
    }
    
    // Prepare audio file
    if (!audioPlayer.prepareAudioFile(audioFileName.c_str())) {
        Serial.println("Failed to prepare audio file!");
        return false;
    }
    
    Serial.printf("Free heap after audio setup: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize WiFi
    if (wifiSSID.isEmpty() || wifiPassword.isEmpty()) {
        Serial.println("WiFi credentials not set!");
        return false;
    }
    
    if (!wifiManager.begin(wifiSSID.c_str(), wifiPassword.c_str())) {
        Serial.println("Failed to connect to WiFi!");
        return false;
    }
    
    Serial.printf("Free heap after WiFi setup: %d bytes\n", ESP.getFreeHeap());
    
    // Initialize web server
    webServer.setAudioPlayer(&audioPlayer);
    webServer.setWiFiManager(&wifiManager);
    webServer.setController(this);
    
    if (!webServer.begin()) {
        Serial.println("Failed to start web server!");
        return false;
    }
    
    // Initialize Bluetooth
    bluetoothSource.setAudioPlayer(&audioPlayer);
    if (!bluetoothSource.begin(bluetoothDeviceName.c_str())) {
        Serial.println("Failed to initialize Bluetooth!");
        return false;
    }
    
    Serial.printf("Free heap after Bluetooth setup: %d bytes\n", ESP.getFreeHeap());
    
    systemReady = true;
    printStartupInfo();
    
    return true;
}

void AudioPlayerController::update() {
    if (!systemReady) return;
    
    // Reset watchdog to prevent timeout
    esp_task_wdt_reset();
    
    // Print status periodically
    if (millis() - lastStatusUpdate > statusUpdateInterval) {
        printSystemStatus();
        lastStatusUpdate = millis();
    }
    
    // Yield to other tasks
    yield();
    delay(50); // Reduced delay for better responsiveness
}

void AudioPlayerController::printSystemStatus() {
    String audioStateStr = (audioPlayer.getState() == PLAYING) ? "PLAYING" : "STANDBY";
    
    if (bluetoothSource.isConnected()) {
        if (audioPlayer.isPlaying()) {
            uint8_t progress = audioPlayer.getProgressPercentage();
            uint32_t played = audioPlayer.getPlayedBytes();
            uint32_t total = audioPlayer.getTotalBytes();
            
            Serial.printf("BT Connected - %s: %d%% (%d/%d bytes)\n", 
                         audioStateStr.c_str(), progress, played, total);
        } else {
            Serial.printf("BT Connected - %s (Ready for commands)\n", audioStateStr.c_str());
        }
    } else {
        Serial.printf("BT Waiting for connection - %s\n", audioStateStr.c_str());
    }
    
    Serial.printf("Free heap: %d bytes | IP: %s | RSSI: %d dBm\n", 
                 ESP.getFreeHeap(), 
                 wifiManager.getLocalIP().c_str(),
                 wifiManager.getRSSI());
}

void AudioPlayerController::printStartupInfo() {
    Serial.println("Setup completed! System is in STANDBY mode.");
    Serial.println("=== System Information ===");
    Serial.printf("Bluetooth Device: %s\n", bluetoothDeviceName.c_str());
    Serial.printf("Audio File: %s\n", audioFileName.c_str());
    Serial.printf("WiFi Network: %s\n", wifiSSID.c_str());
    Serial.printf("IP Address: %s\n", wifiManager.getLocalIP().c_str());
    
    Serial.println("=== Available Endpoints ===");
    String baseURL = webServer.getBaseURL();
    Serial.printf("Web Interface: %s\n", baseURL.c_str());
    Serial.printf("Play Audio:    %splay1\n", baseURL.c_str());
    Serial.printf("Check Status:  %sstatus\n", baseURL.c_str());
    
    Serial.printf("Please pair your Bluetooth speaker with '%s'\n", bluetoothDeviceName.c_str());
}

bool AudioPlayerController::playAudio() {
    return audioPlayer.startPlayback();
}

void AudioPlayerController::stopAudio() {
    audioPlayer.stopPlayback();
}

// Volume control methods (0-100 scale)
void AudioPlayerController::setVolume(uint8_t percentage) {
    // Clamp to 0-100 range
    if (percentage > 100) percentage = 100;
    
    // Convert percentage to 0-127 scale for Bluetooth
    uint8_t bluetoothVolume = mapPercentageToVolume(percentage);
    bluetoothSource.setVolume(bluetoothVolume);
    
    Serial.printf("Volume set to %d%% (BT: %d/127)\n", percentage, bluetoothVolume);
}

uint8_t AudioPlayerController::getVolume() const {
    uint8_t bluetoothVolume = bluetoothSource.getVolume();
    return mapVolumeToPercentage(bluetoothVolume);
}

// Static volume mapping utilities
uint8_t AudioPlayerController::mapVolumeToPercentage(uint8_t value127) {
    // Map 0-127 to 0-100
    // Formula: percentage = (value127 * 100) / 127
    if (value127 == 0) return 0;
    if (value127 >= 127) return 100;
    
    return (uint8_t)((value127 * 100UL) / 127UL);
}

uint8_t AudioPlayerController::mapPercentageToVolume(uint8_t percentage) {
    // Map 0-100 to 0-127
    // Formula: value127 = (percentage * 127) / 100
    if (percentage == 0) return 0;
    if (percentage >= 100) return 127;
    
    return (uint8_t)((percentage * 127UL) / 100UL);
}