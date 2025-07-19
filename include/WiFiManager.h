#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
private:
    String ssid;
    String password;
    bool connected;
    unsigned long connectionTimeout;
    
public:
    WiFiManager();
    ~WiFiManager();
    
    bool begin(const char* ssid, const char* password);
    bool connect();
    bool isConnected() const;
    
    String getLocalIP() const;
    String getSSID() const { return ssid; }
    int getRSSI() const;
    
    void setConnectionTimeout(unsigned long timeout);
    void disconnect();
};

#endif 