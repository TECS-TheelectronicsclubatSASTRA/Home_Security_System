#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

/* ================= USER CONFIG ================= */
const char* ssid     = "DIGISOL";
const char* password = "guna1793";
#define BOTtoken "7401423705:AAFbhiodonXp47CoZ0NgFvsTxPtImV3KMww"
#define CHAT_ID  "1054545199" 

#define PIR_PIN    13
#define FLASH_LED  4

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
unsigned long lastCheckTime;
const unsigned long botRequestDelay = 1500; 
/* =============================================== */

/* ============ AI THINKER PINS ============ */
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

camera_fb_t *fb = NULL;
size_t fb_index = 0;

bool moreDataAvailable() { return (fb != NULL && fb_index < fb->len); }
uint8_t getNextByte() { return (fb != NULL && fb_index < fb->len) ? fb->buf[fb_index++] : 0; }
uint8_t* getNextBuffer() { return NULL; }

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(FLASH_LED, LOW);
  pinMode(PIR_PIN, INPUT);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 20000000; 
  config.pixel_format = PIXFORMAT_JPEG;
  
  // HIGH RES SETTINGS
  config.frame_size = FRAMESIZE_UXGA; // 1600x1200
  config.jpeg_quality = 8;            // 0-63 (lower is higher quality)
  config.fb_count = 1;                // UXGA uses a lot of RAM, keep at 1

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed");
    return;
  }

  // Adjust sensor settings for better high-res quality
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 1);     // -2 to 2
  s->set_contrast(s, 1);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2

  WiFi.begin(ssid, password);
  client.setInsecure();
  while (WiFi.status() != WL_CONNECTED) delay(500);
  bot.sendMessage(CHAT_ID, "📸 High-Resolution System Online", "");
}

void loop() {
  // PIR detection must be fast
  if (digitalRead(PIR_PIN) == HIGH) {
    Serial.println("Motion! Capturing High-Res...");
    captureAndSend();
    delay(5000); 
  }

  if (millis() > lastCheckTime + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    for (int i = 0; i < numNewMessages; i++) {
      String text = bot.messages[i].text;
      if (text == "/flash_on") digitalWrite(FLASH_LED, HIGH);
      if (text == "/flash_off") digitalWrite(FLASH_LED, LOW);
      if (text == "/photo") captureAndSend();
    }
    lastCheckTime = millis();
  }
}

void captureAndSend() {
  // We capture immediately
  fb = esp_camera_fb_get(); 
  
  if (!fb) {
    Serial.println("Capture failed");
    return;
  }

  Serial.printf("Captured: %d bytes\n", fb->len);

  fb_index = 0;
  // This part will take 3-5 seconds because UXGA files are large
  bool sent = bot.sendPhotoByBinary(CHAT_ID, "image/jpeg", fb->len, 
                                    moreDataAvailable, getNextByte, 
                                    getNextBuffer, NULL);

  if (sent) Serial.println("High-Res Sent!");
  
  esp_camera_fb_return(fb);
  fb = NULL;
}