#include <WiFi.h>
#include <WebServer.h>
#include <Arducam_Mega.h>
#include <SPI.h>
#include <SD.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// PIN CONFIG
const int CAM_CS = 5;
#define SD_CS 13
const int POWER_PIN = 4;

Arducam_Mega myCAM(CAM_CS);

const char* ssid = "AERIO4048";
const char* password = "aerio12345678";
const char* AWS_URL = "https://kloy7fchuw4li3dfjsd7joicga0fouov.lambda-url.ap-south-1.on.aws";

WebServer server(80);
String currentPath = "/";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize SPI with explicit pins for LILYGO T-SIM7600
  // CLK=18, MISO=17, MOSI=23 are standard ESP32 SPI pins
  SPI.begin(18, 17, 23, CAM_CS);
  
  // Initialize Arducam
  myCAM.begin();
  myCAM.setMode(MCU2LCD_MODE);
  
  // Initialize SD Card
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed");
  } else {
    Serial.println("SD Card initialized");
  }
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("\n\nConnecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
  
  // Setup web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/take_photo", HTTP_GET, handleTakePhoto);
  server.on("/gallery", HTTP_GET, handleGallery);
  server.on("/photo", HTTP_GET, handlePhotoRequest);
  
  server.begin();
  Serial.println("--- Aerio Server Ready ---");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String html = "<html><body><h1>Aerio Camera</h1>";
  html += "<button onclick=\"fetch('/take_photo')\">");
  html += "Take Photo</button>";
  html += "<button onclick=\"window.location='/gallery'\">");
  html += "View Gallery</button>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleTakePhoto() {
  Serial.println("Taking photo...");
  
  if (myCAM.takePicture(CAM_IMAGE_MODE_320X320, CAM_IMAGE_PIX_FMT_JPG) == 0) {
    uint32_t len = myCAM.getTotalLength();
    String name = "/IMG_" + String(millis()) + ".jpg";
    File file = SD.open(name, FILE_WRITE);
    
    if (file) {
      while (len--) {
        file.write(myCAM.readByte());
      }
      file.close();
      
      Serial.println("Saved: " + name);
      
      // Critical: Give voltage time to recover before WiFi
      Serial.println("Waiting for power to stabilize...");
      delay(1000);
      
      // Now upload to AWS Lambda
      Serial.println("Connecting to AWS Lambda...");
      uploadToAWS(name);
      
      server.send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      server.send(500, "application/json", "{\"error\":\"SD write failed\"}");
    }
  } else {
    server.send(500, "application/json", "{\"error\":\"Camera capture failed\"}");
  }
}

void uploadToAWS(String path) {
  // Check WiFi connectivity before attempting upload
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ERROR: WiFi not connected! Cannot upload to AWS.");
    return;
  }
  
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.println("Failed to open file for AWS upload");
    return;
  }
  
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  
  Serial.println("Uploading to AWS Lambda...");
  if (http.begin(client, AWS_URL)) {
    http.addHeader("Content-Type", "image/jpeg");
    
    // Use stream upload to save RAM
    // For proper multipart upload, we send the file as POST body
    int code = http.sendRequest("POST", &f, f.size());
    
    if (code > 0) {
      Serial.printf("AWS upload code: %d\n", code);
      if (code == 200) {
        Serial.println("AWS upload successful!");
      } else if (code == 502 || code == 503) {
        Serial.println("AWS service error - check Lambda configuration and permissions");
      }
    } else {
      Serial.printf("Upload Error: %s\n", http.errorToString(code).c_str());
      Serial.println("Check WiFi signal, Lambda URL, and network connectivity");
    }
    
    http.end();
  } else {
    Serial.println("Failed to establish HTTP connection to AWS");
  }
  
  f.close();
}

void handleGallery() {
  File root = SD.open("/");
  String html = "<html><head><meta charset=\"UTF-8\"></head><body>";
  html += "<h1>Aerio's SD card Gallery</h1>";
  html += "<button onclick=\"fetch('/take_photo').then(() => location.reload())\">");
  html += "TAKE PHOTO</button>";
  html += "<div style='display:grid; grid-template-columns: repeat(6, 1fr); gap: 10px;'>";
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory() && String(file.name()).endsWith(".jpg")) {
      html += "<div style='border: 2px solid #333; padding: 5px;'>";
      html += "<img src='/photo?file=" + String(file.name()) + "' style='width:100%; cursor:pointer;' onclick=\"alert('Photo: " + String(file.name()) + "')\"/?>";
      html += "<p>" + String(file.name()) + "</p>";
      html += "<button onclick=\"fetch('/photo?file=" + String(file.name()) + "&delete=1').then(() => location.reload())\">");
      html += "Delete</button></div>";
    }
    file = root.openNextFile();
  }
  
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handlePhotoRequest() {
  if (server.hasArg("delete")) {
    String filename = server.arg("file");
    SD.remove("/" + filename);
    server.send(200, "application/json", "{\"status\":\"deleted\"}");
    return;
  }
  
  String filename = server.arg("file");
  File file = SD.open("/" + filename);
  if (file) {
    server.streamFile(file, "image/jpeg");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
}
