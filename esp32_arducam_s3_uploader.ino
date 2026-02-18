/*
 * ESP32 + Arducam Mega 3MP → AWS S3 Cloud Upload
 * 
 * This sketch captures photos from an Arducam Mega camera and uploads them
 * directly to Amazon S3 cloud storage. No SD card required!
 * 
 * BEFORE UPLOADING:
 * 1. Install ESP_S3_Client library: https://github.com/mobizt/ESP_S3_Client
 * 2. Fill in your WiFi credentials below
 * 3. Fill in your AWS credentials (see AWS_S3_INTEGRATION_GUIDE.md)
 * 4. Upload to ESP32 using "ESP32 Dev Module" board setting
 * 
 * Hardware:
 * - ESP32 Dev Module
 * - Arducam Mega 3MP SPI Camera (CS: GPIO 5)
 */

#include <Arducam_Mega.h>
#include <WiFi.h>
#include <ESP_S3_Client.h>

// ============================================================
// CONFIGURATION - FILL THESE IN!
// ============================================================

// WiFi Credentials (Your Home WiFi)
const char* WIFI_SSID = "YOUR_WIFI_SSID_HERE";         // ← Change this!
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD_HERE"; // ← Change this!

// AWS S3 Credentials (Get from AWS IAM Console)
const char* AWS_ACCESS_KEY = "YOUR_ACCESS_KEY_HERE";   // ← Change this!
const char* AWS_SECRET_KEY = "YOUR_SECRET_KEY_HERE";   // ← Change this!
const char* AWS_REGION = "us-east-1";                  // ← Change if needed (e.g., ap-south-1 for Mumbai)
const char* S3_BUCKET_NAME = "esp32-arducam-photos";   // ← Change this!

// Photo Capture Settings
const int CAPTURE_INTERVAL = 30000;  // Capture every 30 seconds (in milliseconds)
const int CAMERA_CS_PIN = 5;         // Arducam CS pin

// ============================================================
// GLOBALS
// ============================================================

Arducam_Mega myCAM(CAMERA_CS_PIN);
S3Client s3;

unsigned long lastCaptureTime = 0;
int photoCount = 0;

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n======================================");
    Serial.println("ESP32 + Arducam → AWS S3 Uploader");
    Serial.println("======================================\n");
    
    // Initialize Camera
    Serial.print("Initializing Arducam Mega... ");
    myCAM.begin();
    Serial.println("OK!");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[ERROR] WiFi Connection Failed!");
        Serial.println("Please check your SSID and Password.");
        while(1) delay(1000);  // Halt
    }
    
    // Initialize S3 Client
    Serial.println("\nInitializing AWS S3 Client...");
    s3.begin(AWS_ACCESS_KEY, AWS_SECRET_KEY, AWS_REGION);
    Serial.println("S3 Client Ready!\n");
    
    Serial.println("======================================");
    Serial.println("System Ready - Starting Auto-Capture");
    Serial.println("======================================\n");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
    // Check if it's time to capture
    if (millis() - lastCaptureTime >= CAPTURE_INTERVAL) {
        captureAndUploadToS3();
        lastCaptureTime = millis();
    }
    
    delay(100);  // Small delay to prevent CPU hogging
}

// ============================================================
// CAPTURE & UPLOAD FUNCTION
// ============================================================

void captureAndUploadToS3() {
    photoCount++;
    
    Serial.println("--------------------------------------------------");
    Serial.print("[Photo #");
    Serial.print(photoCount);
    Serial.println("] Starting Capture...");
    
    // Step 1: Take Picture
    myCAM.takePicture(CAM_IMAGE_MODE_QVGA, CAM_IMAGE_PIX_FMT_JPG);
    uint32_t imageLength = myCAM.getTotalLength();
    
    Serial.print("  Image captured: ");
    Serial.print(imageLength / 1024.0, 2);
    Serial.println(" KB");
    
    if (imageLength == 0) {
        Serial.println("  [ERROR] Image capture failed! Length = 0");
        return;
    }
    
    // Step 2: Generate unique filename
    String filename = "photo_" + String(millis()) + ".jpg";
    Serial.print("  Filename: ");
    Serial.println(filename);
    
    // Step 3: Upload to S3
    Serial.println("  Uploading to AWS S3...");
    
    bool uploadSuccess = s3.uploadFile(
        S3_BUCKET_NAME,
        filename,
        "image/jpeg",
        imageLength,
        // Lambda function for streaming byte-by-byte from Arducam
        [](uint8_t* buffer, size_t bufferSize, size_t index) -> size_t {
            for (size_t i = 0; i < bufferSize; i++) {
                buffer[i] = myCAM.readByte();
            }
            return bufferSize;
        }
    );
    
    // Step 4: Report result
    if (uploadSuccess) {
        Serial.println("  ✓ Upload SUCCESS!");
        Serial.println("\n  Public URL:");
        Serial.print("  https://");
        Serial.print(S3_BUCKET_NAME);
        Serial.print(".s3.");
        Serial.print(AWS_REGION);
        Serial.print(".amazonaws.com/");
        Serial.println(filename);
    } else {
        Serial.println("  ✗ Upload FAILED!");
        Serial.print("  Error: ");
        Serial.println(s3.errorReason());
    }
    
    Serial.println("--------------------------------------------------\n");
}

// ============================================================
// HELPER: Manual Test Function (Optional)
// ============================================================

// Call this from Serial Monitor by sending 'c' to capture on-demand
void serialEvent() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == 'c' || c == 'C') {
            Serial.println("\nManual capture triggered!");
            captureAndUploadToS3();
        }
    }
}
