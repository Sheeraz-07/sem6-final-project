#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"

// Camera pin definition (for AI Thinker)
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

// Define LED pin (Flashlight)
#define FLASHLIGHT_GPIO   4

camera_config_t config;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Set flashlight to ON once in setup()
  pinMode(FLASHLIGHT_GPIO, OUTPUT);
  digitalWrite(FLASHLIGHT_GPIO, HIGH);  // Ensure the flashlight stays on
  Serial.println("Flashlight turned ON");

  // Camera configuration
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    Serial.println("PSRAM found! Using high-quality UXGA resolution");
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("No PSRAM found. Using SVGA resolution");
  }

  // Initialize camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("❌ Camera init failed");
    return;
  }
  Serial.println("Camera initialized successfully");

  // Initialize SD card
  if (!SD_MMC.begin()) {
    Serial.println("❌ SD Card Mount Failed");
    return;
  }

  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("❌ No SD card found");
    return;
  }
  Serial.println("SD Card initialized successfully");
  Serial.println("Ready to capture images...");
}

void loop() {
  // Keep flashlight ON continuously (this ensures the flashlight stays ON)
  digitalWrite(FLASHLIGHT_GPIO, HIGH);
  delay(2000);
  // Capture photo
  Serial.println("📸 Capturing photo...");

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Camera capture failed");
    return;
  }

  // Print image resolution and size info
  Serial.print("Captured image resolution: ");
  switch (config.frame_size) {
    case FRAMESIZE_QVGA:
      Serial.println("QVGA (320x240)");
      break;
    case FRAMESIZE_VGA:
      Serial.println("VGA (640x480)");
      break;
    case FRAMESIZE_SVGA:
      Serial.println("SVGA (800x600)");
      break;
    case FRAMESIZE_XGA:
      Serial.println("XGA (1024x768)");
      break;
    case FRAMESIZE_UXGA:
      Serial.println("UXGA (1600x1200)");
      break;
    default:
      Serial.println("Unknown resolution");
  }

  Serial.print("Captured image size: ");
  Serial.print(fb->len);
  Serial.println(" bytes");

  // Save to SD card with a unique file name (timestamp)
  String fileName = "/photo_" + String(millis()) + ".jpg"; // Unique filename based on millis()
  File file = SD_MMC.open(fileName.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("❌ Failed to open file in write mode");
    esp_camera_fb_return(fb);  // Return frame buffer if file cannot be opened
    return;
  }

  // Write the captured photo to SD card
  if (file.write(fb->buf, fb->len)) {
    Serial.println("✅ Photo saved to " + fileName);
  } else {
    Serial.println("❌ Failed to write data to file");
  }

  // Close the file and release the frame buffer
  file.close();
  esp_camera_fb_return(fb);

  // Add a delay for speed compensation (e.g., 2 seconds)
  Serial.println("Waiting for the next capture...");
  delay(2000);  // Adjust this delay as needed to control the speed of capturing images
}
