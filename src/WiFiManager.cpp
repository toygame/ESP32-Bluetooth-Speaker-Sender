#include "WiFiManager.h"

WiFiManager::WiFiManager() 
    : connected(false), connectionTimeout(30000) {
}

WiFiManager::~WiFiManager() {
    disconnect();
}

bool WiFiManager::begin(const char* ssid, const char* password) {
    this->ssid = ssid;
    this->password = password;
    
    Serial.println("Initializing WiFi Manager...");
    return connect();
}

bool WiFiManager::connect() {
    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > connectionTimeout) {
            Serial.println("\nWiFi connection timeout!");
            connected = false;
            return false;
        }
        
        delay(1000);
        Serial.print(".");
    }
    
    connected = true;
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
    
    return true;
}

bool WiFiManager::isConnected() const {
    return connected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getLocalIP() const {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getRSSI() const {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

void WiFiManager::setConnectionTimeout(unsigned long timeout) {
    this->connectionTimeout = timeout;
}

void WiFiManager::disconnect() {
    if (connected) {
        WiFi.disconnect();
        connected = false;
        Serial.println("WiFi disconnected");
    }
} 