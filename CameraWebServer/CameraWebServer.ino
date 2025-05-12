#include "esp_camera.h"
#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>
#include <WebServer.h>

#define FLASH_PIN 4  // Define FLASH_PIN to GPIO 4 (or any other GPIO pin)

// Wi-Fi credentials
const char* ssid = "Heart Snatcher";  // Corrected SSID
const char* password = "iaminvincible07";

// Web server on port 80
WebServer server(80);

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Camera pin config (for AI Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-CAM Live</title></head><body style='text-align:center;'>";
  html += "<h2>ESP32-CAM Snapshot Viewer</h2>";
  html += "<form action='/capture' method='get'>";
  html += "<button type='submit'>Capture Image</button>";
  html += "</form><br>";

  if (SPIFFS.exists("/captured.jpg")) {
    html += "<img src='/saved-photo?rand=" + String(millis()) + "' alt='Captured Image' width='320'><br><br>";
    html += "<a href='/saved-photo' download='captured.jpg'>Download Last Image</a>";
  } else {
    html += "<p>No image captured yet.</p>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleCapture() {
  digitalWrite(FLASH_PIN, HIGH); // Turn on flashlight
  delay(100); // Let light stabilize

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Camera capture failed");
    digitalWrite(FLASH_PIN, LOW);
    return;
  }

  File file = SPIFFS.open("/captured.jpg", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    server.send(500, "text/plain", "File write failed");
    esp_camera_fb_return(fb);
    digitalWrite(FLASH_PIN, LOW);
    return;
  }

  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  digitalWrite(FLASH_PIN, LOW);

  Serial.println("Image captured and saved");

  // Redirect back to root to show preview
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDownload() {
  if (SPIFFS.exists("/captured.jpg")) {
    File file = SPIFFS.open("/captured.jpg", "r");
    server.streamFile(file, "image/jpeg");
    file.close();
  } else {
    server.send(404, "text/plain", "No image found");
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  unsigned long startAttemptTime = millis();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttemptTime >= 10000) {  // Timeout after 10 seconds
      Serial.println("Failed to connect to Wi-Fi!");
      return;
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  // Start the camera
  startCamera();

  // Set up the flash pin
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, HIGH);

  // Set up server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/capture", HTTP_GET, handleCapture);
  server.on("/saved-photo", HTTP_GET, handleDownload);
  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
