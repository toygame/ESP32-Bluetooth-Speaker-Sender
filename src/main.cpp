#include <Arduino.h>
#include "BluetoothA2DPSource.h"
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// WiFi Configuration
const char* ssid = "your_wifi_ssid";     // แก้ไขเป็นชื่อ WiFi ของคุณ
const char* password = "your_wifi_password"; // แก้ไขเป็นรหัสผ่าน WiFi ของคุณ

// Bluetooth และ WebServer objects
BluetoothA2DPSource a2dp_source;
AsyncWebServer server(80);

// Audio state management
enum AudioState {
    STANDBY,
    PLAYING
};

AudioState currentState = STANDBY;

// ข้อมูลสำหรับ streaming ไฟล์เสียง
File audioFile;
size_t audioFileSize = 0;
size_t audioPosition = 0;
size_t audioDataStart = 0;  // ตำแหน่งเริ่มต้นของ PCM data
size_t audioDataSize = 0;   // ขนาดของ PCM data
bool audioFileOpen = false;

// ข้อมูล WAV format
uint16_t audioChannels = 2;
uint32_t audioSampleRate = 44100;
uint16_t audioBitsPerSample = 16;

// Callback function สำหรับส่งข้อมูลเสียง
int32_t get_sound_data(Frame* data, int32_t frameCount) {
    // ถ้าไม่ใช่สถานะ PLAYING หรือไม่มีไฟล์ ให้ส่งเสียงเงียบ
    if (currentState != PLAYING || !audioFileOpen) {
        for (int i = 0; i < frameCount; i++) {
            data[i].channel1 = 0;
            data[i].channel2 = 0;
        }
        return frameCount;
    }

    // ตรวจสอบว่าเล่นครบแล้วหรือไม่
    if (audioPosition >= (audioDataStart + audioDataSize) || !audioFile.available()) {
        // เล่นจบแล้ว กลับไป STANDBY
        currentState = STANDBY;
        audioFile.close();
        audioFileOpen = false;
        Serial.println("Audio playback completed. Returning to STANDBY mode.");
        
        // ส่งเสียงเงียบ
        for (int i = 0; i < frameCount; i++) {
            data[i].channel1 = 0;
            data[i].channel2 = 0;
        }
        return frameCount;
    }

    int32_t samplesRead = 0;
    uint8_t buffer[4]; // สำหรับ 1 frame (2 channels x 2 bytes)
    
    for (int i = 0; i < frameCount && audioPosition < (audioDataStart + audioDataSize - 3); i++) {
        // อ่านข้อมูล 4 bytes (1 frame = left + right channel)
        size_t bytesRead = audioFile.readBytes((char*)buffer, 4);
        
        if (bytesRead == 4) {
            // แปลงข้อมูล little-endian เป็น 16-bit samples
            int16_t left = (buffer[1] << 8) | buffer[0];
            int16_t right = (buffer[3] << 8) | buffer[2];
            
            data[i].channel1 = left;
            data[i].channel2 = right;
            
            audioPosition += 4;
            samplesRead++;
        } else {
            // ถ้าอ่านไม่ครบ ให้หยุด
            break;
        }
    }
    
    return samplesRead;
}

bool parseWAVHeader() {
    audioFile.seek(0);
    
    // อ่าน RIFF header
    char riff[4];
    audioFile.readBytes(riff, 4);
    if (strncmp(riff, "RIFF", 4) != 0) {
        Serial.println("Not a valid WAV file (missing RIFF)");
        return false;
    }
    
    // ข้าม file size (4 bytes)
    audioFile.seek(8);
    
    // อ่าน WAVE signature
    char wave[4];
    audioFile.readBytes(wave, 4);
    if (strncmp(wave, "WAVE", 4) != 0) {
        Serial.println("Not a valid WAV file (missing WAVE)");
        return false;
    }
    
    // ค้นหา fmt chunk
    while (audioFile.available()) {
        char chunkID[4];
        uint32_t chunkSize;
        
        audioFile.readBytes(chunkID, 4);
        audioFile.readBytes((char*)&chunkSize, 4);
        
        if (strncmp(chunkID, "fmt ", 4) == 0) {
            // อ่านข้อมูล format
            uint16_t audioFormat;
            audioFile.readBytes((char*)&audioFormat, 2);
            audioFile.readBytes((char*)&audioChannels, 2);
            audioFile.readBytes((char*)&audioSampleRate, 4);
            
            // ข้าม byte rate (4) และ block align (2)
            audioFile.seek(audioFile.position() + 6);
            audioFile.readBytes((char*)&audioBitsPerSample, 2);
            
            Serial.printf("WAV Format: %d channels, %d Hz, %d bits\n", 
                         audioChannels, audioSampleRate, audioBitsPerSample);
            
            if (audioFormat != 1) {
                Serial.println("Only PCM format supported!");
                return false;
            }
            
            if (audioChannels != 2 || audioSampleRate != 44100 || audioBitsPerSample != 16) {
                Serial.println("WAV file must be: 44.1kHz, 16-bit, stereo");
                return false;
            }
            
            // ข้ามข้อมูลที่เหลือใน fmt chunk
            audioFile.seek(audioFile.position() + (chunkSize - 16));
        }
        else if (strncmp(chunkID, "data", 4) == 0) {
            // เจอ data chunk แล้ว
            audioDataStart = audioFile.position();
            audioDataSize = chunkSize;
            
            Serial.printf("PCM data starts at position %d, size %d bytes\n", 
                         audioDataStart, audioDataSize);
            return true;
        }
        else {
            // ข้าม chunk อื่นๆ
            audioFile.seek(audioFile.position() + chunkSize);
        }
    }
    
    Serial.println("No data chunk found in WAV file");
    return false;
}

bool prepareAudioFile() {
    Serial.println("Preparing WAV file for playback...");
    
    audioFile = SPIFFS.open("/play1.wav", "r");
    if (!audioFile) {
        Serial.println("Failed to open audio file!");
        return false;
    }
    
    audioFileSize = audioFile.size();
    Serial.printf("WAV file size: %d bytes\n", audioFileSize);
    
    // Parse WAV header
    if (!parseWAVHeader()) {
        audioFile.close();
        return false;
    }
    
    audioFile.close();
    Serial.printf("WAV file prepared. PCM data: %d bytes\n", audioDataSize);
    return true;
}

bool startAudioPlayback() {
    if (currentState == PLAYING) {
        Serial.println("Audio already playing!");
        return false;
    }
    
    Serial.println("Starting audio playback...");
    
    // เปิดไฟล์ใหม่สำหรับการเล่น
    audioFile = SPIFFS.open("/play1.wav", "r");
    if (!audioFile) {
        Serial.println("Failed to open audio file for playback!");
        return false;
    }
    
    // เริ่มที่ตำแหน่ง PCM data
    audioPosition = audioDataStart;
    audioFile.seek(audioPosition);
    audioFileOpen = true;
    
    // เปลี่ยนสถานะเป็น PLAYING
    currentState = PLAYING;
    
    Serial.println("Audio playback started!");
    return true;
}

void setupWiFi() {
    Serial.println("Setting up WiFi...");
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setupWebServer() {
    Serial.println("Setting up Web Server...");
    
    // API endpoint สำหรับเล่นเสียง
    server.on("/play1", HTTP_GET, [](AsyncWebServerRequest *request){
        if (startAudioPlayback()) {
            request->send(200, "text/plain", "Audio playback started!");
        } else {
            request->send(400, "text/plain", "Failed to start audio playback or already playing");
        }
    });
    
    // Status endpoint
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String status = (currentState == PLAYING) ? "PLAYING" : "STANDBY";
        String response = "Status: " + status + "\n";
        response += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
        response += "WiFi IP: " + WiFi.localIP().toString() + "\n";
        request->send(200, "text/plain", response);
    });
    
    // Root endpoint
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<html><body>";
        html += "<h1>ESP32 Audio Player</h1>";
        html += "<p><a href='/play1'>Play Audio</a></p>";
        html += "<p><a href='/status'>Check Status</a></p>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });
    
    server.begin();
    Serial.println("Web Server started!");
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Bluetooth Audio Server Starting...");
    
    // แสดงข้อมูล memory ที่มีอยู่
    Serial.printf("Free heap before SPIFFS: %d bytes\n", ESP.getFreeHeap());
    
    // เริ่มต้น SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }
    Serial.println("SPIFFS mounted successfully");
    
    // แสดงรายการไฟล์ใน SPIFFS
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("Files in SPIFFS:");
    while (file) {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
    
    Serial.printf("Free heap after SPIFFS: %d bytes\n", ESP.getFreeHeap());
    
    // เตรียมไฟล์เสียง
    if (!prepareAudioFile()) {
        Serial.println("Cannot proceed without valid audio file!");
        return;
    }
    
    // ตั้งค่า WiFi
    setupWiFi();
    
    // ตั้งค่า WebServer
    setupWebServer();
    
    Serial.printf("Free heap after WiFi setup: %d bytes\n", ESP.getFreeHeap());
    
    // ตั้งค่า Bluetooth A2DP Source
    a2dp_source.set_auto_reconnect(true);
    a2dp_source.set_data_callback_in_frames(get_sound_data);
    a2dp_source.set_volume(100);

    Serial.println("Starting Bluetooth A2DP Source...");
    a2dp_source.start("AL-01");
    
    Serial.println("Setup completed! System is in STANDBY mode.");
    Serial.println("=== Available endpoints ===");
    Serial.printf("Web Interface: http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.printf("Play Audio:    http://%s/play1\n", WiFi.localIP().toString().c_str());
    Serial.printf("Check Status:  http://%s/status\n", WiFi.localIP().toString().c_str());
    Serial.println("Please pair your Bluetooth speaker with 'AL-01'");
}

void loop() {
    // ตรวจสอบสถานะการเชื่อมต่อ
    static unsigned long lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 10000) { // ตรวจสอบทุก 10 วินาที
        String stateStr = (currentState == PLAYING) ? "PLAYING" : "STANDBY";
        
        if (a2dp_source.is_connected()) {
            if (currentState == PLAYING) {
                uint32_t playedBytes = audioPosition - audioDataStart;
                uint32_t totalBytes = audioDataSize;
                uint32_t percentage = (playedBytes * 100) / totalBytes;
                
                Serial.printf("BT Connected - %s: %d%% (%d/%d bytes)\n", 
                             stateStr.c_str(), percentage, playedBytes, totalBytes);
            } else {
                Serial.printf("BT Connected - %s (Ready for commands)\n", stateStr.c_str());
            }
        } else {
            Serial.printf("BT Waiting for connection - %s\n", stateStr.c_str());
        }
        
        Serial.printf("Free heap: %d bytes | IP: %s\n", 
                     ESP.getFreeHeap(), WiFi.localIP().toString().c_str());
        lastStatusCheck = millis();
    }
    
    delay(100);
}