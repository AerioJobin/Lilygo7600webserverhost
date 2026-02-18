# AWS Configuration Details

⚠️ **IMPORTANT: KEEP THIS FILE SECURE AND NEVER COMMIT TO PUBLIC REPOSITORIES**

## AWS S3 Bucket Information

**Bucket Name:** `esp32-arducam-photos-aerio`

**Region:** `ap-south-1` (Asia Pacific - Mumbai)

**Bucket URL:** `https://esp32-arducam-photos-aerio.s3.ap-south-1.amazonaws.com`

**Public Access:** Enabled (objects are publicly readable)

## IAM User Credentials

**IAM User Name:** `esp32-camera-uploader`

**Access Key ID:** `AKIA2RMWQWXP7RPW64DQ`

**Secret Access Key:** `hvljdXsbPndOicio3xQgdiR8taEevMFM`

**Policy Name:** `ESP32-S3-Bucket-Upload-Policy`

**Permissions:**
- `s3:PutObject` - Upload objects to S3
- `s3:PutObjectAcl` - Set object access control lists

**Resource:** `arn:aws:s3:::esp32-arducam-photos-aerio/*`

## Usage in ESP32 Code

Add these credentials to your Arduino sketch:

```cpp
// AWS Credentials
const char* AWS_ACCESS_KEY_ID = "AKIA2RMWQWXP7RPW64DQ";
const char* AWS_SECRET_ACCESS_KEY = "hvljdXsbPndOicio3xQgdiR8taEevMFM";
const char* AWS_BUCKET_NAME = "esp32-arducam-photos-aerio";
const char* AWS_REGION = "ap-south-1";
const char* AWS_HOST = "s3.ap-south-1.amazonaws.com";
```

## Public Photo URL Format

Photos uploaded will be accessible at:

```
https://esp32-arducam-photos-aerio.s3.ap-south-1.amazonaws.com/[filename].jpg
```

Example:
```
https://esp32-arducam-photos-aerio.s3.ap-south-1.amazonaws.com/photo_20260218_223045.jpg
```

## Security Notes

1. **Never commit this file to public repositories**
2. **Rotate access keys regularly**
3. **Delete unused access keys**
4. **Monitor AWS CloudTrail for suspicious activity**
5. **Consider using environment variables instead of hardcoding credentials**
6. **For production, use AWS IoT Core with certificate-based authentication**

## AWS Console Access

- **S3 Console:** https://console.aws.amazon.com/s3/buckets/esp32-arducam-photos-aerio
- **IAM Console:** https://console.aws.amazon.com/iam/home#/users/esp32-camera-uploader

---

**Created:** February 18, 2026
**Last Updated:** February 18, 2026
