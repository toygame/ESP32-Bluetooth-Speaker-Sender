#include <Arduino.h>
#include "AudioPlayerController.h"

// Global controller instance
AudioPlayerController controller;

void setup() {
    // Configure the controller
    controller.setWiFiCredentials("CTNT_2G_1FL", "successful");
    controller.setBluetoothDeviceName("AL-01");
    controller.setAudioFile("/play1.wav");
    controller.setStatusUpdateInterval(10000); // 10 seconds
    controller.setVolume(80); // Volume: 0-100% (mapped to Bluetooth 0-127)
    
    // Initialize the system
    if (!controller.begin()) {
        Serial.println("Failed to initialize system!");
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    controller.update();
}