# Aerio SD Gallery: ESP32 + Arducam Mega 3MP

A high-performance, mobile-responsive web server for capturing and managing photos using an Arducam Mega 3MP SPI camera and an SD Card. This project features a modern dark-mode UI, asynchronous photo capture, and an integrated gallery with delete functionality.

## 🚀 Features

- **Real-time Capture**: Trigger the Arducam Mega via a web interface
- **SD Storage**: Automatically saves images with unique timestamps using `millis()`
- **Modern UI**: A clean, CSS-grid-based gallery with a Slate-dark theme
- **Dual SPI Bus**: Optimized performance by isolating the Camera on VSPI and the SD Card on HSPI
- **File Management**: View full-resolution images and delete unwanted photos directly from the browser
- **Access Point Mode**: No router needed; the ESP32 creates its own Wi-Fi network

## 🛠 Hardware Required

| Component | Description |
|-----------|-------------|
| **Microcontroller** | ESP32 (Tested on LilyGo / DevKit V1) |
| **Camera** | Arducam Mega 3MP SPI Camera |
| **Storage** | MicroSD Card Module (Formatted to FAT32) |
| **Power** | 5V 2A power supply recommended (SPI peripherals are power-hungry) |

## 📌 Pinout Mapping

### Arducam
| Function | ESP32 Pin |
|----------|----------|
| CS | GPIO 5 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |

### SD Card
| Function | ESP32 Pin |
|----------|----------|
| CS | GPIO 13 |
| SCK | GPIO 14 |
| MISO | GPIO 2 |
| MOSI | GPIO 15 |

### Power
| Function | ESP32 Pin |
|----------|----------|
| Peripheral Enable | GPIO 4 |

## 💻 Installation

1. **Libraries**: Install the following via the Arduino Library Manager:
   - `Arducam_Mega`
   - `WiFi`, `WebServer`, `SD`, & `FS` (Standard ESP32 libs)

2. **Configuration**: Update the `ssid` and `password` variables in the `.ino` file

3. **Upload**: Use the "ESP32 Dev Module" board setting

4. **Connect**: Join the Wi-Fi network "Aerio4048" and navigate to http://192.168.4.1

## 📝 Technical Problem Report

This project underwent several iterations to resolve hardware-software timing conflicts. Below is the documentation of the challenges and solutions discovered.

### 1. SPI Bus Contention

**Problem**: Initially, both the Arducam and the SD card shared the default SPI bus. This caused collision errors where the SD card would fail to initialize if the camera was active.

**Solution**: Implemented Dual SPI isolation. By defining a custom SPIClass for the SD card on the HSPI pins while keeping the Camera on the default VSPI bus, both peripherals operate independently without signal interference.

### 2. The "Broken Image" / 0KB File Bug

**Problem**: Images appeared in the gallery as broken icons. Serial logs showed tiny file sizes (3KB to 9KB), which indicated the transfer was cutting off prematurely.

**Solution**: Advanced "Burst/Buffered Reads" were clashing with the SPI clock timing of this specific hardware. Reverting to a Byte-by-Byte read loop (`file.write(myCAM.readByte())`) ensured every single byte from the camera's internal buffer was successfully flushed to the SD card.

### 3. Web Server URI Mismatch

**Problem**: The browser requested `/IMG_123.jpg`, but the SD library sometimes indexed it as `IMG_123.jpg` (without the slash). This caused 404 errors for the images despite them existing on the card.

**Solution**: Created a Path Normalizer logic. The server now checks for the file in three ways: exactly as requested, with a forced leading slash, and without a leading slash.

### 4. Browser MIME Type Sensitivity

**Problem**: Files would send, but mobile browsers wouldn't display them, treating them as binary data.

**Solution**: Added explicit HTTP Header Locking. By sending `server.sendHeader("Content-Type", "image/jpeg")` before streaming, the browser is forced to interpret the byte stream as a JPEG image.

### 5. SPI Clock Stability

**Problem**: High-speed SPI (20MHz+) caused the SD card to time out when handled alongside Wi-Fi tasks.

**Solution**: Clocked the SD SPI bus down to 4MHz during initialization to ensure stable data transfer while the ESP32 managed the asynchronous web requests.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](.github/CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.
