#include "AudioPlayer.h"
#include <ESP.h>

AudioPlayer::AudioPlayer() 
    : audioFileSize(0), audioPosition(0), audioDataStart(0), audioDataSize(0),
      audioFileOpen(false), currentState(STANDBY),
      audioChannels(2), audioSampleRate(44100), audioBitsPerSample(16) {
}

AudioPlayer::~AudioPlayer() {
    if (audioFileOpen) {
        audioFile.close();
    }
}

bool AudioPlayer::begin() {
    Serial.println("Initializing AudioPlayer...");
    
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return false;
    }
    
    Serial.println("SPIFFS mounted successfully");
    
    // List files in SPIFFS
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("Files in SPIFFS:");
    while (file) {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
    
    return true;
}

bool AudioPlayer::prepareAudioFile(const char* filename) {
    Serial.printf("Preparing audio file: %s\n", filename);
    
    audioFile = SPIFFS.open(filename, "r");
    if (!audioFile) {
        Serial.println("Failed to open audio file!");
        return false;
    }
    
    audioFileSize = audioFile.size();
    Serial.printf("Audio file size: %d bytes\n", audioFileSize);
    
    if (!parseWAVHeader()) {
        audioFile.close();
        return false;
    }
    
    audioFile.close();
    Serial.printf("Audio file prepared. PCM data: %d bytes\n", audioDataSize);
    return true;
}

bool AudioPlayer::parseWAVHeader() {
    audioFile.seek(0);
    
    // Read RIFF header
    char riff[4];
    audioFile.readBytes(riff, 4);
    if (strncmp(riff, "RIFF", 4) != 0) {
        Serial.println("Not a valid WAV file (missing RIFF)");
        return false;
    }
    
    // Skip file size (4 bytes)
    audioFile.seek(8);
    
    // Read WAVE signature
    char wave[4];
    audioFile.readBytes(wave, 4);
    if (strncmp(wave, "WAVE", 4) != 0) {
        Serial.println("Not a valid WAV file (missing WAVE)");
        return false;
    }
    
    // Find fmt chunk
    while (audioFile.available()) {
        char chunkID[4];
        uint32_t chunkSize;
        
        audioFile.readBytes(chunkID, 4);
        audioFile.readBytes((char*)&chunkSize, 4);
        
        if (strncmp(chunkID, "fmt ", 4) == 0) {
            // Read format information
            uint16_t audioFormat;
            audioFile.readBytes((char*)&audioFormat, 2);
            audioFile.readBytes((char*)&audioChannels, 2);
            audioFile.readBytes((char*)&audioSampleRate, 4);
            
            // Skip byte rate (4) and block align (2)
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
            
            // Skip remaining data in fmt chunk
            audioFile.seek(audioFile.position() + (chunkSize - 16));
        }
        else if (strncmp(chunkID, "data", 4) == 0) {
            // Found data chunk
            audioDataStart = audioFile.position();
            audioDataSize = chunkSize;
            
            Serial.printf("PCM data starts at position %d, size %d bytes\n", 
                         audioDataStart, audioDataSize);
            return true;
        }
        else {
            // Skip other chunks
            audioFile.seek(audioFile.position() + chunkSize);
        }
    }
    
    Serial.println("No data chunk found in WAV file");
    return false;
}

bool AudioPlayer::startPlayback() {
    if (currentState == PLAYING) {
        Serial.println("Audio already playing!");
        return false;
    }
    
    Serial.println("Starting audio playback...");
    
    // Open file for playback
    audioFile = SPIFFS.open("/play1.wav", "r");
    if (!audioFile) {
        Serial.println("Failed to open audio file for playback!");
        return false;
    }
    
    // Start at PCM data position
    audioPosition = audioDataStart;
    audioFile.seek(audioPosition);
    audioFileOpen = true;
    
    // Change state to PLAYING
    currentState = PLAYING;
    
    Serial.println("Audio playback started!");
    return true;
}

void AudioPlayer::stopPlayback() {
    if (currentState == PLAYING) {
        currentState = STANDBY;
        if (audioFileOpen) {
            audioFile.close();
            audioFileOpen = false;
        }
        Serial.println("Audio playback stopped.");
    }
}

int32_t AudioPlayer::getAudioData(AudioFrame* data, int32_t frameCount) {
    // Check memory health
    if (ESP.getFreeHeap() < 5000) {
        Serial.printf("CRITICAL: Very low memory in getAudioData: %d bytes\n", ESP.getFreeHeap());
        // Force stop playback to free resources
        stopPlayback();
    }
    
    // If not playing or no file open, send silence
    if (currentState != PLAYING || !audioFileOpen) {
        for (int i = 0; i < frameCount; i++) {
            data[i].channel1 = 0;
            data[i].channel2 = 0;
        }
        return frameCount;
    }

    // Check if playback is complete
    if (audioPosition >= (audioDataStart + audioDataSize) || !audioFile.available()) {
        // Playback finished, return to STANDBY
        currentState = STANDBY;
        audioFile.close();
        audioFileOpen = false;
        Serial.println("Audio playback completed. Returning to STANDBY mode.");
        
        // Send silence
        for (int i = 0; i < frameCount; i++) {
            data[i].channel1 = 0;
            data[i].channel2 = 0;
        }
        return frameCount;
    }

    int32_t samplesRead = 0;
    uint8_t buffer[4]; // For 1 frame (2 channels x 2 bytes)
    
    for (int i = 0; i < frameCount && audioPosition < (audioDataStart + audioDataSize - 3); i++) {
        // Read 4 bytes (1 frame = left + right channel)
        size_t bytesRead = audioFile.readBytes((char*)buffer, 4);
        
        if (bytesRead == 4) {
            // Convert little-endian to 16-bit samples
            int16_t left = (buffer[1] << 8) | buffer[0];
            int16_t right = (buffer[3] << 8) | buffer[2];
            
            data[i].channel1 = left;
            data[i].channel2 = right;
            
            audioPosition += 4;
            samplesRead++;
        } else {
            // If can't read full frame, stop
            break;
        }
    }
    
    return samplesRead;
}

uint32_t AudioPlayer::getPlayedBytes() const {
    if (currentState == PLAYING && audioPosition >= audioDataStart) {
        return audioPosition - audioDataStart;
    }
    return 0;
}

uint32_t AudioPlayer::getTotalBytes() const {
    return audioDataSize;
}

uint8_t AudioPlayer::getProgressPercentage() const {
    if (audioDataSize == 0) return 0;
    uint32_t played = getPlayedBytes();
    return (played * 100) / audioDataSize;
} 