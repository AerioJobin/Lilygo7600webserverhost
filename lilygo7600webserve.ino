#include <WiFi.h>
#include <WebServer.h>
#include <Arducam_Mega.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <HTTPClient.h>

// -------- PIN CONFIGURATION (Your Working Pins) --------
const int CAM_CS = 5; 
const int CAM_SCK = 18;
const int CAM_MISO = 19;
const int CAM_MOSI = 23;

#define SD_SCK  14
#define SD_MISO 2
#define SD_MOSI 15
#define SD_CS   13

const int POWER_PIN = 4;

// -------- OBJECTS --------
SPIClass sdSPI(HSPI); 
Arducam_Mega myCAM(CAM_CS);
WebServer server(80);

const char* ssid = "AERIO4048";
const char* password = "aerio12345678-";
const char* AWS_API_URL = "https://3ifs2w7u2j.execute-api.ap-south-1.amazonaws.com/upload";

// --- 1. THE WORKING FILE LOADER ---
bool loadFromSD(String path) {
  // Try opening with/without leading slash
  File dataFile = SD.open(path, "r");
  if (!dataFile && path.startsWith("/")) dataFile = SD.open(path.substring(1), "r");
  if (!dataFile && !path.startsWith("/")) dataFile = SD.open("/" + path, "r");

  if (!dataFile || dataFile.isDirectory()) return false;

  // Set JPEG header so the browser knows what to do
  server.streamFile(dataFile, "image/jpeg");
  dataFile.close();
  return true;
}

// --- 2. YOUR ORIGINAL WORKING CAPTURE LOGIC ---

// --- AWS UPLOAD FUNCTION ---
bool uploadToAws(String filePath) {
  File f = SD.open(filePath, FILE_READ);
  if (!f) {
    Serial.println("Failed to open file for AWS upload");
    return false;
  }
  
  size_t size = f.size();
  if (size == 0) {
    f.close();
    return false;
  }
  
  uint8_t *buf = (uint8_t*)malloc(size);
  if (!buf) {
    Serial.println("No RAM for buffer");
    f.close();
    return false;
  }
  
  f.read(buf, size);
  f.close();
  
  HTTPClient http;
  http.begin(AWS_API_URL);
  http.addHeader("Content-Type", "image/jpeg");
  
  int httpCode = http.POST(buf, size);
  free(buf);
  
  if (httpCode > 0) {
    Serial.printf("AWS upload code: %d\n", httpCode);
  } else {
    Serial.printf("AWS upload failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return httpCode == 200;
}
void handleCapture() {
  // Back to the basics: take the picture
  uint8_t status = myCAM.takePicture(CAM_IMAGE_MODE_320X320, CAM_IMAGE_PIX_FMT_JPG);
  
  if (status == 0) {
    uint32_t len = myCAM.getTotalLength();
    String fileName = "/IMG_" + String(millis()) + ".jpg";
    
    File file = SD.open(fileName, FILE_WRITE);
    if (file) {
      // Reverting to your original byte-by-byte read loop since it worked
      while (len--) {
        file.write(myCAM.readByte()); 
      }
      file.close();
      Serial.println("Saved: " + fileName);
            uploadToAws(fileName);  // Upload to AWS S3
      server.send(200, "text/plain", "Success");
    } else {
      server.send(500, "text/plain", "SD Write Failed");
    }
  } else {
    server.send(500, "text/plain", "Camera Error");
  }
}

// --- 3. DELETE LOGIC ---
void handleDelete() {
  String path = server.arg("path");
  if (SD.remove(path) || SD.remove("/" + path) || SD.remove(path.substring(1))) {
    server.send(200, "text/plain", "Deleted");
  } else {
    server.send(500, "text/plain", "Fail");
  }
}

// --- 4. THE CLEAN UI ---
void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body{background:#0f172a; color:#f8fafc; font-family:sans-serif; margin:0; padding:20px; text-align:center;}";
  html += ".btn{background:#3b82f6; color:white; padding:15px; border:none; border-radius:12px; font-weight:bold; cursor:pointer; width:100%; max-width:400px; margin-bottom:20px;}";
  html += ".gallery{display:grid; grid-template-columns: repeat(auto-fill, minmax(140px, 1fr)); gap:15px;}";
  html += ".card{background:#1e293b; border-radius:12px; padding:10px; border:1px solid #334155;}";
  html += "img{width:100%; height:120px; object-fit:cover; border-radius:8px; background:#000;}";
  html += ".fname{font-size:10px; color:#94a3b8; display:block; margin:8px 0; overflow:hidden;}";
  html += ".del{background:#ef4444; color:white; border:none; padding:6px; width:100%; border-radius:6px; cursor:pointer; font-size:11px;}";
  html += "</style></head><body>";

  html += "<h2>Aerio SD Gallery</h2>";
  html += "<button class='btn' onclick=\"this.innerText='Capturing...'; fetch('/capture').then(()=>location.reload())\">📸 TAKE PHOTO</button>";
  html += "<div class='gallery'>";

  File root = SD.open("/");
  File file = root.openNextFile();
  int count = 0;
  
  while (file) {
    String name = String(file.name());
    if (name.endsWith(".jpg") || name.endsWith(".JPG")) {
      count++;
      // Force leading slash for the web path
      String webPath = name.startsWith("/") ? name : "/" + name;
      html += "<div class='card'>";
      html += "<img src='" + webPath + "' onclick=\"window.open('" + webPath + "')\">";
      html += "<span class='fname'>" + name + "</span>";
      html += "<button class='del' onclick=\"if(confirm('Delete?')){fetch('/delete?path=" + webPath + "').then(()=>location.reload())}\">Delete</button></div>";
    }
    file = root.openNextFile();
  }
  
  if(count == 0) html += "<p>No photos found.</p>";
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  delay(1000);

  // Initialize SD Card
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSPI)) {
    Serial.println("SD Card: FAIL");
  }

  // Initialize Camera
  SPI.begin(CAM_SCK, CAM_MISO, CAM_MOSI, CAM_CS); 
  if (myCAM.begin() != 0) {
    Serial.println("Camera: FAIL");
  }

  WiFi.softAP(ssid, password);
  
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/delete", handleDelete);
  
  server.onNotFound([]() {
    if (!loadFromSD(server.uri())) {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.begin();
  Serial.println("\n--- Aerio Server Ready ---");
}

void loop() {
  server.handleClient();
}
