# Quick Setup Checklist for AWS S3 Upload

Follow these steps to get your ESP32 uploading photos to AWS S3:

## ☐ Step 1: AWS Account Setup

1. ☐ Log into [AWS Console](https://console.aws.amazon.com/)
2. ☐ Create S3 bucket (e.g., `esp32-arducam-photos`)
3. ☐ Disable "Block all public access" (if you want public URLs)
4. ☐ Add bucket policy for public read access (see guide)
5. ☐ Create IAM user `esp32-uploader`
6. ☐ Attach `AmazonS3FullAccess` policy (or custom policy)
7. ☐ Generate Access Key & Secret Key
8. ☐ **Save keys immediately!** (You can't retrieve them later)

## ☐ Step 2: Install Arduino Library

1. ☐ Open Arduino IDE
2. ☐ Go to **Sketch** > **Include Library** > **Manage Libraries**
3. ☐ Search for **"ESP_S3_Client"** by Mobizt
4. ☐ Click **Install**

## ☐ Step 3: Configure the Arduino Sketch

1. ☐ Open `esp32_arducam_s3_uploader.ino` in Arduino IDE
2. ☐ Fill in the following at the top of the file:

```cpp
const char* WIFI_SSID = "YourWiFiName";            // Your home WiFi name
const char* WIFI_PASSWORD = "YourWiFiPassword";    // Your WiFi password

const char* AWS_ACCESS_KEY = "AKIAIOSFODNN7EXAMPLE";  // From AWS IAM
const char* AWS_SECRET_KEY = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";  // From AWS IAM
const char* AWS_REGION = "us-east-1";              // Your bucket region
const char* S3_BUCKET_NAME = "esp32-arducam-photos";  // Your bucket name
```

## ☐ Step 4: Upload to ESP32

1. ☐ Connect ESP32 to computer via USB
2. ☐ Select **Tools** > **Board** > **ESP32 Dev Module**
3. ☐ Select correct **Port** (COM port where ESP32 is connected)
4. ☐ Click **Upload** button (right arrow icon)
5. ☐ Wait for "Done uploading" message

## ☐ Step 5: Test & Monitor

1. ☐ Open **Tools** > **Serial Monitor**
2. ☐ Set baud rate to **115200**
3. ☐ Watch for connection and upload messages
4. ☐ Look for "Upload SUCCESS!" messages
5. ☐ Copy the public URL and open it in your browser

## ✅ Success!

If you see:
```
--------------------------------------------------
[Photo #1] Starting Capture...
  Image captured: 45.67 KB
  Filename: photo_123456.jpg
  Uploading to AWS S3...
  ✓ Upload SUCCESS!

  Public URL:
  https://esp32-arducam-photos.s3.us-east-1.amazonaws.com/photo_123456.jpg
--------------------------------------------------
```

**Congratulations! Your ESP32 is now uploading to the cloud! 🎉**

---


## ☐ Step 6: Configure AWS Lambda Function

The Lambda function acts as the "Brain" for your camera. It receives images from the ESP32 and uploads them to S3.

1. ☐ Go to [AWS Lambda Console](https://console.aws.amazon.com/lambda)
2. ☐ Create a Lambda function named `camera-upload-handler`
3. ☐ Set Auth type to **NONE** (public access)
4. ☐ Enable **CORS** with `Allow origin: *`
5. ☐ Create/Attach IAM role with **S3FullAccess** policy
6. ☐ Deploy the `lambda_function.py` code (see repo file)
7. ☐ Copy your **Function URL** and update ESP32 code:
   ```cpp
   const char* AWS_URL = "https://your-lambda-url.aws/";
   ```
8. ☐ Deploy ESP32 code and test

**Important:** Ensure the closing brace on line 24 of lambda_function.py is present - missing it causes 502 errors!


## Troubleshooting

- **WiFi won't connect?** Check SSID and password spelling
- **403 Forbidden error?** Verify IAM permissions and Access Keys
- **Upload timeout?** Check your internet connection speed
- **Need more help?** See [AWS_S3_INTEGRATION_GUIDE.md](AWS_S3_INTEGRATION_GUIDE.md)
