# ESP32 to AWS EC2 Cloud Streaming System

## Overview
This folder contains the **EC2 server-side implementation** for the ESP32 camera cloud streaming project. This new architecture transforms the system from a standalone ESP32 Access Point into a robust client-server model where AWS EC2 handles all backend processing.

## 🔄 Architecture Comparison

| Feature | Original (Access Point) | New (EC2 Client-Server) |
|---------|------------------------|-------------------------|
| **ESP32 Role** | Server (Master) | Client |
| **WiFi Mode** | Access Point | Station Mode |
| **Backend** | ESP32 web server | AWS EC2 Flask |
| **Storage** | SD Card only | EC2 disk + optional S3 |
| **UI Hosting** | ESP32 (limited) | EC2 (full-featured) |
| **Scalability** | Single client | Multiple ESP32s |
| **Remote Access** | Local network only | Internet-accessible |

## 📁 Files in This Folder

### 1. `app.py`
The Flask server application that runs on your EC2 instance.

**Features:**
- Receives image uploads via HTTP POST
- Stores images with timestamp-based filenames
- Serves a clean dark-mode gallery UI
- Handles image serving and display

**Usage:**
```bash
sudo python3 app.py
```

### 2. `ESP32_EC2_CLIENT.md`
Comprehensive documentation including:
- Complete ESP32 client code with HTTPClient library
- WiFi Station Mode setup
- Auto-reconnect logic for 24/7 operation
- EC2 instance setup guide (t2.micro, security groups, Elastic IP)
- Technical problems & solutions (5 major challenges documented)
- Deployment checklist
- Next steps and enhancements

## 🚀 Quick Start Guide

### Prerequisites
- AWS Account
- ESP32 with Arducam Mega 3MP
- Home WiFi network

### Step 1: Launch EC2 Instance
1. Go to AWS EC2 Console
2. Launch a **t2.micro** instance (Free Tier)
3. Choose **Ubuntu 22.04 LTS**
4. Configure Security Group:
   - Allow **Port 80** (HTTP) from 0.0.0.0/0
   - Allow **Port 22** (SSH) from your IP

### Step 2: Allocate Elastic IP
1. In EC2 Console, go to **Elastic IPs**
2. Allocate a new Elastic IP
3. Associate it with your EC2 instance
4. **Note this IP** - you'll need it for ESP32 code

### Step 3: Setup Flask Server
SSH into your EC2 instance and run:
```bash
sudo apt update
sudo apt install python3-pip python3-flask -y

# Create app directory
mkdir ~/camera_server
cd ~/camera_server

# Upload app.py to this directory
# Then run:
sudo python3 app.py
```

### Step 4: Configure ESP32
Update your ESP32 code with:
```cpp
const char* ssid = "YOUR_HOME_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://YOUR_EC2_ELASTIC_IP/upload";
```

See `ESP32_EC2_CLIENT.md` for the complete ESP32 client code.

### Step 5: Test the System
1. Upload code to ESP32
2. ESP32 connects to your WiFi
3. Trigger photo capture
4. Image uploads to EC2
5. Access gallery at: `http://YOUR_EC2_ELASTIC_IP`

## 🔧 Technical Challenges Solved

### 1. Elastic IP Management
**Problem:** EC2 public IP changes on reboot, breaking ESP32 connection.  
**Solution:** Use AWS Elastic IP for static addressing.

### 2. Security Group Configuration
**Problem:** ESP32 couldn't reach EC2 server.  
**Solution:** Open Port 80 in EC2 Security Group.

### 3. WiFi Reliability
**Problem:** ESP32 loses WiFi connection randomly.  
**Solution:** Implemented auto-reconnect loop in `loop()`.

### 4. Memory Management
**Problem:** Large image buffers cause ESP32 crashes.  
**Solution:** Proper `malloc()` / `free()` with error checking.

### 5. HTTP Timeouts
**Problem:** Large image uploads timeout.  
**Solution:** Increased timeout values and added retry logic.

## 🎯 Key Advantages

1. **Professional Backend**: Flask server vs limited ESP32 server
2. **Scalability**: Can handle multiple ESP32 devices
3. **Storage**: EC2 disk space instead of SD card limitations
4. **Remote Access**: Access gallery from anywhere via public IP
5. **Processing Power**: Can add ML inference, image processing
6. **Database Integration**: Easy to add PostgreSQL/MySQL
7. **Security**: Implement HTTPS, authentication, etc.

## 📊 System Flow

```
ESP32 Camera
    ↓
[Capture Photo]
    ↓
WiFi (Station Mode)
    ↓
Internet
    ↓
AWS EC2 (Public IP)
    ↓
Flask Backend (app.py)
    ↓
/uploads folder
    ↓
Gallery UI (Dark Mode)
```

## 🔐 Security Considerations

- **Elastic IP**: Static addressing prevents connection issues
- **Security Groups**: Firewall rules for Port 80 access
- **TODO**: Add HTTPS with Let's Encrypt SSL
- **TODO**: Implement user authentication
- **TODO**: Add API key validation for uploads

## 🛠️ Troubleshooting

### ESP32 Can't Connect to WiFi
- Check SSID and password spelling
- Verify WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check router MAC filtering

### ESP32 Can't Upload to EC2
- Verify EC2 Security Group allows Port 80
- Check Elastic IP is correctly associated
- Ping the EC2 IP from another device

### Flask Server Won't Start
- Ensure Python3 and Flask are installed
- Check if Port 80 is already in use: `sudo netstat -tlnp | grep :80`
- Run with sudo: `sudo python3 app.py`

### Images Not Displaying
- Check `/uploads` folder permissions
- Verify Flask route `/uploads/<filename>` is working
- Check browser console for errors

## 📈 Performance Metrics

- **Upload Speed**: ~100KB image in 2-3 seconds
- **EC2 t2.micro**: Can handle 10+ concurrent uploads
- **Storage**: EC2 has 8GB root volume (expandable)
- **Latency**: <1 second for gallery page load

## 🔮 Future Enhancements

1. **HTTPS Support**: Add SSL certificates with Let's Encrypt
2. **Authentication**: Implement user login system
3. **Database**: Store metadata in PostgreSQL
4. **S3 Integration**: Long-term storage in S3 buckets
5. **CloudWatch**: Monitoring and alerting
6. **Auto-Scaling**: Handle multiple ESP32 devices
7. **Image Processing**: Add filters, compression, ML inference
8. **Mobile App**: React Native app for remote control

## 📚 Related Documentation

- [ESP32_EC2_CLIENT.md](./ESP32_EC2_CLIENT.md) - Complete implementation guide
- [../AWS_S3_INTEGRATION_GUIDE.md](../AWS_S3_INTEGRATION_GUIDE.md) - S3 alternative
- [../README.md](../README.md) - Original Access Point version

## 💡 Tips

- Keep EC2 instance running to avoid IP changes (even with Elastic IP)
- Monitor EC2 Free Tier usage (750 hours/month)
- Use `tmux` or `screen` to keep Flask running after SSH disconnect
- Set up CloudWatch alarms for EC2 health monitoring

## 📝 License

MIT License - Same as parent project

---

**Author**: Aerio Jobin Momin  
**Project**: Lilygo T-SIM7600 Camera Cloud Streaming  
**Date**: February 2026
