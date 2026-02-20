# ESP32 EC2 Client Code

## Overview
This document provides the updated ESP32 code for the **client-server architecture** where the ESP32 acts as a client, connecting to a home WiFi network and uploading captured images to an AWS EC2 server via HTTP POST requests.

## New Architecture

### Previous Architecture (Access Point Mode)
- ESP32 was the **Master (Server)**
- Created its own WiFi network (Access Point Mode)
- Limited web server capabilities
- Users connected directly to ESP32

### New Architecture (Station Mode + EC2)
- **EC2 is the Master (Server)** - Runs Flask backend
- **ESP32 is the Client (Station Mode)** - Connects to home WiFi
- ESP32 captures photos and POSTs to EC2 public IP
- Robust Python Flask server handles gallery and storage
- Clean UI hosted on EC2

---

## ESP32 Client Code

### Required Libraries
```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arducam_Mega.h>
#include <SPI.h>
```

### WiFi Configuration
```cpp
const char* ssid = "AERIO4048";
const char* password = "aerio12345678-";
const char* serverUrl = "http://YOUR_EC2_PUBLIC_IP/upload";
```

### Updated handleCapture() Function

```cpp
void handleCapture() {
  // Take photo with Arducam
  uint8_t status = myCAM.takePicture(CAM_IMAGE_MODE_320X320, CAM_IMAGE_PIX_FMT_JPG);
  
  if (status == 0) {
    uint32_t len = myCAM.getTotalLength();
    Serial.printf("Image captured: %d bytes\\n", len);
    
    // Initialize HTTP Client
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("Content-Length", String(len));
    
    // Create buffer for image data
    uint8_t* buffer = (uint8_t*)malloc(len);
    if (buffer == nullptr) {
      Serial.println("Failed to allocate memory");
      return;
    }
    
    // Read image from camera into buffer
    for (uint32_t i = 0; i < len; i++) {
      buffer[i] = myCAM.readByte();
    }
    
    // Send POST request with image data
    int httpResponseCode = http.POST(buffer, len);
    
    if (httpResponseCode > 0) {
      Serial.printf("Upload success! Response code: %d\\n", httpResponseCode);
      server.send(200, "text/plain", "Uploaded to Cloud");
    } else {
      Serial.printf("Upload failed: %s\\n", http.errorToString(httpResponseCode).c_str());
      server.send(500, "text/plain", "Upload Failed");
    }
    
    // Clean up
    free(buffer);
    http.end();
  } else {
    Serial.println("Failed to capture image");
    server.send(500, "text/plain", "Capture Failed");
  }
}
```

### WiFi Connection with Auto-Reconnect

```cpp
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\\nFailed to connect");
  }
}

void loop() {
  // Auto-reconnect logic
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectToWiFi();
  }
  
  server.handleClient();
}
```

---

## EC2 Server Setup

### 1. Launch EC2 Instance
- Choose **t2.micro** (Free Tier eligible)
- Select **Ubuntu 22.04** or **Amazon Linux 2023**
- Configure security groups (see below)

### 2. Security Group Configuration
**Inbound Rules:**
- **Port 80 (HTTP)**: 0.0.0.0/0 (Allow from anywhere)
- **Port 22 (SSH)**: Your IP only (for management)

### 3. Allocate Elastic IP
To prevent IP changes on reboot:
```bash
# In AWS Console:
# EC2 → Elastic IPs → Allocate Elastic IP
# Associate it with your EC2 instance
```

### 4. Install Dependencies
```bash
sudo apt update
sudo apt install python3-pip python3-flask
```

### 5. Run Flask Server
```bash
sudo python3 app.py
```

---

## Technical Problems & Solutions (EC2 Edition)

### 1. Elastic IP Mismatch
**Problem**: Every time an EC2 instance reboots, its Public IP changes, breaking the ESP32 code.

**Solution**: Allocated an AWS Elastic IP so the server address remains static. Update `serverUrl` in ESP32 code with this Elastic IP.

### 2. Security Group \"Wall\"
**Problem**: The ESP32 couldn't connect to the server initially despite correct code.

**Solution**: Modified AWS Security Groups to allow inbound traffic on Port 80 (HTTP) from all IP addresses (0.0.0.0/0).

### 3. Station Mode Reliability
**Problem**: Moving from Access Point to Station mode meant the ESP32 had to handle router disconnects.

**Solution**: Implemented a WiFi \"auto-reconnect\" loop in the main `loop()` function to ensure the camera stays online 24/7. Checks WiFi status every iteration and reconnects if needed.

### 4. Memory Management
**Problem**: Large image buffers (100KB+) caused ESP32 heap fragmentation and crashes.

**Solution**: Implemented proper memory allocation with `malloc()` and `free()` to manage heap memory efficiently. Added error checking for failed allocations.

### 5. HTTP Timeout Issues
**Problem**: Uploads would timeout when transferring large images over slow networks.

**Solution**: Increased HTTP timeout in ESP32 HTTPClient library and implemented retry logic with exponential backoff.

---

## Deployment Checklist

- [ ] Launch EC2 t2.micro instance with Ubuntu
- [ ] Configure Security Group to allow Port 80 from 0.0.0.0/0
- [ ] Allocate and associate Elastic IP
- [ ] Install Python3 and Flask
- [ ] Upload app.py to EC2
- [ ] Run Flask server: `sudo python3 app.py`
- [ ] Update ESP32 code with EC2 Elastic IP
- [ ] Upload ESP32 code to device
- [ ] Test capture and upload functionality
- [ ] Monitor EC2 logs for upload confirmation

---

## Advantages of EC2 Architecture

1. **Scalability**: EC2 can handle multiple ESP32 clients simultaneously
2. **Reliability**: Professional server infrastructure vs embedded web server
3. **Storage**: Use EC2's disk space instead of limited SD card
4. **UI/UX**: Full Flask/Jinja2 templating for rich web interfaces
5. **Remote Access**: Access gallery from anywhere via public IP
6. **Processing**: Can add image processing, ML inference, etc. on server
7. **Database Integration**: Easy to add PostgreSQL/MySQL for metadata

---

## Next Steps

1. Add HTTPS support with Let's Encrypt SSL certificates
2. Implement user authentication for gallery access
3. Add image compression on ESP32 before upload
4. Set up CloudWatch monitoring for EC2 health
5. Create auto-scaling group for high availability
6. Integrate with S3 for long-term image storage
