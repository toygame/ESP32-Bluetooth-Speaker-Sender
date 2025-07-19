# ESP32 Bluetooth Speaker Sender

🎵 **ESP32-based Bluetooth A2DP Source with Web API Control**

เครื่องส่งเสียงผ่าน Bluetooth ที่ควบคุมผ่าน Web API สำหรับ ESP32 โดยใช้ไฟล์เสียง WAV จากระบบไฟล์ SPIFFS

![ESP32](https://img.shields.io/badge/ESP32-Compatible-blue)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-orange)
![License](https://img.shields.io/badge/License-MIT-green)

## ✨ Features

- 🎧 **Bluetooth A2DP Source** - เล่นเสียงไปยังลำโพง/หูฟัง Bluetooth
- 🌐 **Web API Control** - ควบคุมการเล่นเสียงผ่าน HTTP API
- 📁 **SPIFFS File System** - จัดเก็บไฟล์เสียงใน Flash memory
- 🔄 **Smart Playback** - เล่น 1 รอบแล้วหยุดอัตโนมัติ
- 📊 **Real-time Status** - ตรวจสอบสถานะการทำงานแบบ real-time
- 🎛️ **Volume Control** - ปรับระดับเสียงได้
- 💾 **Memory Optimized** - ใช้ streaming แทนการโหลดทั้งไฟล์

## 🛠️ Hardware Requirements

- **ESP32 Development Board** (NodeMCU-32S หรือ compatible)
- **WiFi Network** สำหรับ Web API
- **Bluetooth Speaker/Headphones** รองรับ A2DP
- **MicroSD Card** (optional สำหรับไฟล์เสียงขนาดใหญ่)

## 📋 Audio File Requirements

- **Format**: WAV (PCM)
- **Sample Rate**: 44.1 kHz
- **Channels**: Stereo (2 channels)
- **Bit Depth**: 16-bit
- **Size**: ≤ 2MB (ขึ้นอยู่กับ SPIFFS partition)

## 🚀 Installation

### 1. Clone Repository
```bash
git clone https://github.com/toygame/ESP32-Bluetooth-Speaker-Sender.git
cd ESP32-Bluetooth-Speaker-Sender
```

### 2. Install PlatformIO
```bash
# ติดตั้ง PlatformIO CLI
pip install platformio

# หรือใช้ VS Code Extension
# ค้นหา "PlatformIO IDE" ใน VS Code Extensions
```

### 3. Configure WiFi Credentials
แก้ไขไฟล์ `src/main.cpp`:
```cpp
// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 4. Prepare Audio File
1. วางไฟล์ `play1.wav` ในโฟลเดอร์ `data/`
2. ตรวจสอบให้ไฟล์ตรงตาม requirements ข้างต้น

### 5. Build & Upload
```bash
# Clean และ build
pio run --target clean
pio run

# Upload filesystem (ไฟล์เสียง)
pio run --target uploadfs

# Upload firmware
pio run --target upload

# Monitor serial output
pio device monitor
```

## 🎮 Usage

### การเริ่มต้นใช้งาน

1. **Power On**: เปิดเครื่อง ESP32
2. **WiFi Connection**: รอการเชื่อมต่อ WiFi
3. **Bluetooth Pairing**: จับคู่ลำโพง Bluetooth กับ "AL-01"
4. **Get IP Address**: ดู IP address ใน Serial Monitor

### Web Interface

เปิด browser ไปที่: `http://[ESP32_IP_ADDRESS]/`

```
ESP32 Audio Player
- Play Audio
- Check Status
```

## 🔗 API Endpoints

### GET `/`
**หน้าหลัก Web Interface**
```
Response: HTML page with controls
```

### GET `/play1`
**เล่นเสียง 1 รอบ**
```bash
curl http://192.168.1.100/play1
```
```
Response: "Audio playback started!" (200)
         "Failed to start..." (400)
```

### GET `/status`
**ตรวจสอบสถานะ**
```bash
curl http://192.168.1.100/status
```
```
Response:
Status: STANDBY
Free Heap: 234567 bytes
WiFi IP: 192.168.1.100
```

## 📊 System States

| State | Description | Audio Output |
|-------|-------------|--------------|
| **STANDBY** | รอคำสั่ง | เสียงเงียบ |
| **PLAYING** | กำลังเล่นเสียง | PCM audio data |

### State Transitions
```
STANDBY --[API /play1]--> PLAYING --[ครบ 1 รอบ]--> STANDBY
```

## 🔧 Configuration

### Partition Table (`audio_partitions.csv`)
```csv
# Name,   Type, SubType, Offset,  Size,     Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1E0000,
spiffs,   data, spiffs,  0x1F0000,0x200000,
```

### Build Flags (`platformio.ini`)
```ini
build_flags =
    -Os                                    # Optimize for size
    -D CORE_DEBUG_LEVEL=5                  # Debug level
    -D CONFIG_ASYNC_TCP_MAX_ACK_TIME=5000  # TCP timeout
    -D CONFIG_ASYNC_TCP_RUNNING_CORE=1     # CPU core binding
    -D CONFIG_ASYNC_TCP_STACK_SIZE=4096    # Stack size
    -ffunction-sections                    # Linker optimization
    -fdata-sections
    -Wl,--gc-sections
    -Wl,--strip-all
```

## 🎵 Audio Processing Flow

```
WAV File (SPIFFS) → Parse Header → Extract PCM Data → 
Bluetooth A2DP → Codec (SBC) → Bluetooth Speaker
```

1. **File Reading**: Stream แบบ real-time จาก SPIFFS
2. **Header Parsing**: วิเคราะห์ WAV header เพื่อหา PCM data
3. **Frame Processing**: แปลง PCM เป็น frames สำหรับ A2DP
4. **Bluetooth Transmission**: ส่งผ่าน SBC codec

## 🐛 Troubleshooting

### ปัญหาที่พบบ่อย

#### 1. **Cannot connect to WiFi**
```
Solution: ตรวจสอบ SSID และ password ใน src/main.cpp
```

#### 2. **SPIFFS Mount Failed**
```bash
# Upload filesystem ใหม่
pio run --target uploadfs
```

#### 3. **Audio file not found**
```
Solution: 
1. ใส่ไฟล์ play1.wav ในโฟลเดอร์ data/
2. รัน pio run --target uploadfs
```

#### 4. **Bluetooth pairing failed**
```
Solution:
1. ลบ device "AL-01" ออกจากลำโพง
2. Reset ESP32
3. Pair ใหม่
```

#### 5. **Firmware too large**
```
Error: The program size (1612389 bytes) is greater than maximum allowed

Solution: ใช้ partition table ที่มี app ขนาดใหญ่ขึ้น (audio_partitions.csv)
```

#### 6. **No sound output**
```
Checklist:
✓ ไฟล์เสียงเป็น 44.1kHz, 16-bit, stereo PCM
✓ Bluetooth speaker paired และ connected
✓ เรียก API /play1 เพื่อเริ่มเล่น
✓ ตรวจสอบ volume level
```

### Debug Commands

```bash
# ดู log แบบ real-time
pio device monitor

# ตรวจสอบขนาด firmware
pio run --target size

# ดู partition table
esptool.py --port /dev/ttyUSB0 read_flash 0x8000 0x1000 partition_table.bin
```

## 📁 Project Structure

```
ESP32-Bluetooth-Speaker-Sender/
├── src/
│   └── main.cpp              # Main application code
├── data/
│   └── play1.wav            # Audio file (add your own)
├── include/
│   └── README               # Include directory
├── lib/
│   └── README               # Library directory
├── audio_partitions.csv     # Custom partition table
├── platformio.ini           # PlatformIO configuration
└── README.md               # This file
```

## 🔬 Technical Details

### Memory Usage
- **Flash**: ~1.6MB (firmware) + 2MB (SPIFFS)
- **RAM**: ~58KB (runtime)
- **Audio Buffer**: 4 bytes per frame (streaming)

### Bluetooth Specifications
- **Profile**: A2DP (Advanced Audio Distribution Profile)
- **Codec**: SBC (Sub-band Coding)
- **Sample Rate**: 44.1 kHz
- **Bit Rate**: ~320 kbps

### Network Stack
- **WiFi**: 802.11 b/g/n
- **HTTP Server**: ESPAsyncWebServer
- **TCP**: AsyncTCP with custom configuration

## 📚 Dependencies

```ini
lib_deps =
    https://github.com/pschatzmann/ESP32-A2DP.git
    https://github.com/pschatzmann/arduino-audio-tools.git
    FS
    ESP32Async/AsyncTCP
    ESP32Async/ESPAsyncWebServer
```

## 🤝 Contributing

1. Fork the [repository](https://github.com/toygame/ESP32-Bluetooth-Speaker-Sender)
2. Create feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit changes (`git commit -m 'Add AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) by Phil Schatzmann
- [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) by ESP32Async
- [Arduino Audio Tools](https://github.com/pschatzmann/arduino-audio-tools) by Phil Schatzmann

## 📞 Support

หากมีปัญหาหรือข้อสงสัย:
- 🐛 [Create an Issue](https://github.com/toygame/ESP32-Bluetooth-Speaker-Sender/issues)
- 💬 [Discussions](https://github.com/toygame/ESP32-Bluetooth-Speaker-Sender/discussions)
- 📧 Email: thanapon.toy@hotmail.com

---

⭐ **ถ้าโปรเจคนี้มีประโยชน์ กรุณา Star ให้ด้วยนะครับ!** ⭐ 