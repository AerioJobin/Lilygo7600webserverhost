# Transitioning ESP32 + Arducam Mega to AWS S3

This guide provides a step-by-step process to move your local SD storage and web server to a cloud-based solution using Amazon S3.

## 1. AWS Configuration (S3 & IAM)

### Step 1: Create an S3 Bucket
1. Log in to the [AWS Management Console](https://console.aws.amazon.com/).
2. Navigate to **S3** and click **Create bucket**.
3. Name your bucket (e.g., `esp32-arducam-photos`) and select a region.
4. **Public Access**: Uncheck "Block all public access" if you want anyone to view photos via URL.
5. Under **Bucket Policy**, add a policy to allow public read access (be careful with security!):
   ```json
   {
       "Version": "2012-10-17",
       "Statement": [
           {
               "Sid": "PublicReadGetObject",
               "Effect": "Allow",
               "Principal": "*",
               "Action": "s3:GetObject",
               "Resource": "arn:aws:s3:::your-bucket-name/*"
           }
       ]
   }
   ```

### Step 2: Create an IAM User for ESP32
1. Navigate to **IAM** > **Users** > **Create user**.
2. Name it `esp32-uploader`.
3. Select **Attach policies directly** and choose **AmazonS3FullAccess** (or a custom scoped policy).
4. Create the user and navigate to the **Security credentials** tab.
5. Create an **Access key** (CLI/Local code use case).
6. **Save your Access Key ID and Secret Access Key immediately.**

---

## 2. Library Recommendations

For efficient HTTPS and AWS Signature V4 handling on ESP32, I recommend:
- **[ESP_S3_Client](https://github.com/mobizt/ESP_S3_Client)**: A powerful and efficient library that handles AWS V4 signatures, chunked uploads, and HTTPS natively for ESP32. It's much simpler than manual signing.

---

## 3. Modified Arduino Code (Streaming to S3)

Instead of saving to an SD card, we stream the Arducam buffer directly to the AWS HTTP POST request.

```cpp
#include <Arducam_Mega.h>
#include <WiFi.h>
#include <ESP_S3_Client.h>

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// AWS Credentials
const char* aws_access_key = "YOUR_ACCESS_KEY";
const char* aws_secret_key = "YOUR_SECRET_KEY";
const char* aws_region = "us-east-1"; // e.g., us-east-1
const char* bucket_name = "esp32-arducam-photos";

// Camera setup
Arducam_Mega myCAM(5); // CS Pin 5

S3Client s3;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    myCAM.begin();
    
    // Configure S3 Client
    s3.begin(aws_access_key, aws_secret_key, aws_region);
}

void captureAndUpload() {
    myCAM.takePicture(CAM_IMAGE_MODE_QVGA, CAM_IMAGE_PIX_FMT_JPG);
    uint32_t len = myCAM.getTotalLength();
    
    String fileName = "photo_" + String(millis()) + ".jpg";
    
    Serial.println("Uploading to S3...");
    
    // Start S3 Upload
    if (s3.uploadFile(bucket_name, fileName, "image/jpeg", len, 
        [](uint8_t* buffer, size_t size, size_t index) {
            // Streaming callback: Read directly from Arducam buffer
            for (size_t i = 0; i < size; i++) {
                buffer[i] = myCAM.readByte();
            }
            return size;
        })) {
        Serial.println("Upload Success!");
        Serial.print("Public URL: https://");
        Serial.print(bucket_name);
        Serial.print(".s3.");
        Serial.print(aws_region);
        Serial.print(".amazonaws.com/");
        Serial.println(fileName);
    } else {
        Serial.printf("Upload Failed: %s
", s3.errorReason().c_str());
    }
}

void loop() {
    // Trigger capture every 30 seconds
    captureAndUpload();
    delay(30000);
}
```

---

## 4. Key Logic Changes

1. **Dual SPI Bus**: You can now disable the HSPI bus used for the SD card (pins 14, 2, 15, 13) to save power or use them for other peripherals.
2. **Path Normalizer**: No longer needed as AWS handles URIs predictably.
3. **MIME Types**: Handled in the `s3.uploadFile` parameter (`"image/jpeg"`).
4. **Direct Streaming**: The `s3.uploadFile` function uses a callback. Inside this callback, we use your reliable `myCAM.readByte()` loop to fill the transmission buffer, ensuring zero data loss and no requirement for massive RAM allocation for the image.


---

## 5. Manual AWS Signature V4 Implementation (If You Don't Use ESP_S3_Client)

If you prefer a library-free approach (not recommended for production), here's a minimal AWS Signature V4 signing example:

```cpp
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>

// HMAC-SHA256 Helper
void hmacSHA256(const char* key, size_t keyLen, const char* data, size_t dataLen, uint8_t* result) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key, keyLen);
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)data, dataLen);
    mbedtls_md_hmac_finish(&ctx, result);
    mbedtls_md_free(&ctx);
}

String generateAWSSignature(String secretKey, String dateStamp, String region, String service) {
    uint8_t kDate[32], kRegion[32], kService[32], kSigning[32];
    String key = "AWS4" + secretKey;
    hmacSHA256(key.c_str(), key.length(), dateStamp.c_str(), dateStamp.length(), kDate);
    hmacSHA256((char*)kDate, 32, region.c_str(), region.length(), kRegion);
    hmacSHA256((char*)kRegion, 32, service.c_str(), service.length(), kService);
    hmacSHA256((char*)kService, 32, "aws4_request", 14, kSigning);
    // Return as hex string for Authorization header...
}
```

**Note**: This is complex and error-prone. Use **ESP_S3_Client** instead.

---

## 6. Testing Your Setup

1. Flash the modified code to your ESP32.
2. Monitor Serial output to see the upload status and generated public URLs.
3. Open the public URL in your browser:
   ```
   https://your-bucket-name.s3.your-region.amazonaws.com/photo_12345.jpg
   ```

---

## 7. Security Best Practices

⚠️ **Important Warnings:**
- **Never hardcode AWS credentials in production firmware.** Consider:
  - Using AWS IoT Core with certificate-based authentication.
  - Storing credentials in NVS (Non-Volatile Storage) and programming them separately.
- **Limit IAM permissions:** Instead of `AmazonS3FullAccess`, use a custom policy:
  ```json
  {
      "Version": "2012-10-17",
      "Statement": [
          {
              "Effect": "Allow",
              "Action": "s3:PutObject",
              "Resource": "arn:aws:s3:::your-bucket-name/*"
          }
      ]
  }
  ```
- **Enable CloudFront** for S3 to add HTTPS, caching, and better access control.
- **Rotate Access Keys** periodically.

---

## 8. Cost Considerations

AWS charges for:
- **Storage**: ~$0.023/GB/month (S3 Standard)
- **PUT Requests**: $0.005 per 1,000 requests
- **Data Transfer Out**: Free for first 100GB/month, then $0.09/GB

Example: Uploading 1 photo every 30 seconds:
- Photos per day: 2,880
- At 200KB/photo = ~576 MB/day
- Monthly storage: ~17GB = **~$0.40/month**
- Monthly PUT requests: ~86,400 = **~$0.43/month**

**Total: ~$0.83/month** (assuming minimal downloads)

---

## 9. Troubleshooting

### Issue: "403 Forbidden" Error
- Check IAM user has `PutObject` permission.
- Verify AWS credentials are correct.
- Ensure bucket name and region match.

### Issue: "Connection Timeout"
- Check WiFi connection stability.
- Increase timeout in `s3.setTimeout(30000)`.
- Use a WiFi signal booster if ESP32 is far from router.

### Issue: "Partial Upload / Corrupted Images"
- This is rare with ESP_S3_Client's chunked uploads.
- Verify `myCAM.getTotalLength()` returns the correct image size.
- Add retry logic in case of network interruptions.

---

## 10. Next Steps

- **Add a web dashboard**: Build a simple React/Vue app that lists S3 objects.
- **Implement motion detection**: Only upload when motion is detected.
- **Add metadata**: Store timestamp, location, or sensor data in S3 object metadata.
- **Use AWS Lambda**: Trigger image processing (resize, facial recognition) on upload.

---

## Libraries to Install

```bash
# Arduino IDE Library Manager
1. Arducam_Mega (already installed)
2. ESP_S3_Client by Mobizt
```

Or via PlatformIO:
```ini
lib_deps =
    arducam/Arducam Mega@^1.0.0
    mobizt/ESP_S3_Client@^1.0.0
```

---

**Good luck with your cloud migration!** Feel free to open an issue if you encounter any problems.
