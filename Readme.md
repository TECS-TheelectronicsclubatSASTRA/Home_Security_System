# 📸 ESP32-CAM Home Security System with Telegram Alerts

A motion-triggered security camera built around an **ESP32-CAM (AI-Thinker)** and a **PIR motion sensor**. When motion is detected, the camera captures a photo and pushes it straight to your phone through a **Telegram bot** — anywhere in the world, as long as the device has Wi-Fi.

No cloud subscription. No monthly fee. No app to install beyond Telegram itself.

![Wiring diagram of the ESP32-CAM home security system](images/wiring-diagram.jpeg)

---

## Table of Contents

1. [What It Does](#1-what-it-does)
2. [Features](#2-features)
3. [How It Works](#3-how-it-works)
4. [Bill of Materials](#4-bill-of-materials)
5. [Pin Connections](#5-pin-connections)
6. [Understanding the PIR Sensor](#6-understanding-the-pir-sensor)
7. [Setting Up the Telegram Bot](#7-setting-up-the-telegram-bot)
8. [Software Setup (Arduino IDE)](#8-software-setup-arduino-ide)
9. [The Code](#9-the-code)
10. [Uploading to the ESP32-CAM](#10-uploading-to-the-esp32-cam)
11. [First Run and Testing](#11-first-run-and-testing)
12. [Building the Enclosure](#12-building-the-enclosure)
13. [Heat Management](#13-heat-management)
14. [Installation and Placement](#14-installation-and-placement)
15. [Troubleshooting](#15-troubleshooting)
16. [Tuning and Optimisation](#16-tuning-and-optimisation)
17. [Results](#17-results)
18. [Future Improvements](#18-future-improvements)
19. [Privacy and Legal Note](#19-privacy-and-legal-note)
20. [Credits](#20-credits)

---

## 1. What It Does

Someone walks past your gate, your driveway, or your street. The PIR sensor picks up the change in infrared heat from their body and pulls its output pin HIGH. That signal goes into GPIO 13 of the ESP32-CAM. The ESP32 wakes up, grabs a JPEG frame from the OV2640 camera, opens an HTTPS connection to the Telegram Bot API, and uploads the image to your personal chat.

Total time from motion to photo on your phone: roughly **3–6 seconds** on a decent Wi-Fi connection.

Because Telegram handles the delivery, there is no port forwarding, no static IP, no DDNS, and no router configuration. The device only makes **outbound** connections, which is both simpler and considerably safer.

---

## 2. Features

- 🔔 Instant photo alerts pushed to Telegram
- 🌍 Works from anywhere — the phone receiving alerts does not need to be on the same network
- 🔒 No inbound ports opened; outbound HTTPS only
- 💸 Zero running cost (Telegram Bot API is free)
- 📦 Fits in a small plastic box
- ♻️ Runs continuously for months without intervention
- ⚙️ Adjustable sensitivity, detection range and cooldown

---

## 3. How It Works

```
   ┌──────────────┐  motion detected   ┌──────────────┐
   │  PIR Sensor  │ ─────────────────▶ │  ESP32-CAM   │
   │  (HC-SR501)  │   3.3V HIGH pulse  │  (AI-Thinker)│
   └──────────────┘    on GPIO 13      └──────┬───────┘
                                              │ capture JPEG
                                              ▼
                                       ┌──────────────┐
                                       │   OV2640     │
                                       │   Camera     │
                                       └──────┬───────┘
                                              │ frame buffer
                                              ▼
                                    ┌────────────────────┐
                                    │  Wi-Fi (2.4 GHz)   │
                                    └─────────┬──────────┘
                                              │ HTTPS POST
                                              ▼
                                  ┌────────────────────────┐
                                  │  api.telegram.org      │
                                  │  /bot<TOKEN>/sendPhoto │
                                  └─────────┬──────────────┘
                                            │
                                            ▼
                                    📱 Your Telegram chat
```

The image is never stored on an SD card in the basic version — it goes from the camera's frame buffer straight into a multipart HTTP request body. That keeps the code lean and avoids SD card wear.

---

## 4. Bill of Materials

| # | Component | Qty | Notes |
|---|-----------|-----|-------|
| 1 | ESP32-CAM module (AI-Thinker) | 1 | Must include the OV2640 camera ribbon |
| 2 | PIR motion sensor (HC-SR501) | 1 | HC-SR312 / AM312 also works at 3.3V |
| 3 | FTDI / USB-to-TTL adapter (CP2102, CH340 or FT232RL) | 1 | **Only needed for programming** |
| 4 | 5V DC power adapter, 2A minimum | 1 | 1A often causes brownouts — do not skimp |
| 5 | Jumper wires (male–female, female–female) | ~10 | |
| 6 | 470 µF / 10V electrolytic capacitor | 1 | Across 5V and GND — strongly recommended |
| 7 | Plastic enclosure box | 1 | Any household box with a lid |
| 8 | Micro SD card (optional) | 1 | Only if you want local backup of photos |
| 9 | Soldering iron, drill/hot needle, glue gun | — | For the enclosure |

> 💡 **On the ESP32-CAM:** there is no USB port on the board. You *must* use an external USB-to-TTL adapter to flash it, or buy the ESP32-CAM-MB programmer shield which clips onto the bottom and gives you a micro-USB socket. The shield is the easier route for beginners.

---

## 5. Pin Connections

### 5.1 AI-Thinker ESP32-CAM Pinout Reference

Holding the board with the camera facing you and the antenna at the top:

```
        ┌─────────────────────────┐
        │      [ ANTENNA ]        │
   5V ──┤                         ├── 3V3 / 5V
  GND ──┤                         ├── IO16
 IO12 ──┤      ESP32-CAM          ├── IO0
 IO13 ──┤      AI-THINKER         ├── GND
 IO15 ──┤                         ├── VCC
 IO14 ──┤                         ├── U0R (IO3)
  IO2 ──┤                         ├── U0T (IO1)
  IO4 ──┤                         ├── GND
        │   [ CAMERA + uSD ]      │
        └─────────────────────────┘
```

**Pins that are already taken and must not be reused:** GPIO 0, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 34, 35, 36, 39 (camera bus), and GPIO 4 (onboard white flash LED), GPIO 33 (onboard red status LED).

**Safely usable GPIOs:** 12, 13, 14, 15, 2, 16 — and 1/3 if you are not using serial. We use **GPIO 13**.

> ⚠️ Avoid GPIO 12 for the PIR. It is a strapping pin (MTDI); if it is held HIGH during boot the chip tries to run the flash at 1.8 V and will fail to start. GPIO 13 has no such restriction, which is exactly why the diagram uses it.

### 5.2 Wiring Table — PIR Sensor to ESP32-CAM

| HC-SR501 Pin | Connect To | Wire colour in diagram |
|---|---|---|
| **VCC** | 5V rail (from the DC adapter, same rail as ESP32-CAM 5V) | Red |
| **OUT** | **GPIO 13** on ESP32-CAM | Green |
| **GND** | Common GND rail | Black |

### 5.3 Wiring Table — Power

| Source | Destination |
|---|---|
| 5V adapter **+** | ESP32-CAM `5V` pin **and** PIR `VCC` |
| 5V adapter **−** | ESP32-CAM `GND` **and** PIR `GND` |
| 470 µF capacitor **+** | 5V rail |
| 470 µF capacitor **−** | GND rail |

**Everything must share a common ground.** If the PIR is powered from one source and the ESP32 from another without a ground link, the trigger signal will be meaningless and you will get either constant false alarms or nothing at all.

### 5.4 Wiring Table — Programming Only (FTDI)

Connect these *only while flashing*, then disconnect GPIO 0 before normal use.

| FTDI Pin | ESP32-CAM Pin |
|---|---|
| 5V (or 3.3V, see note) | 5V |
| GND | GND |
| TX | U0R (GPIO 3) |
| RX | U0T (GPIO 1) |
| — | **GPIO 0 → GND** (jumper wire, puts board in flash mode) |

> 📌 Note on FTDI voltage: set the adapter jumper to **5V** and feed the `5V` pin. The board has an onboard regulator. Powering the `3V3` pin from a 3.3V FTDI works but many adapters cannot supply the ~300 mA peak the Wi-Fi radio needs, causing upload failures. If uploads keep failing, power the board from the wall adapter and use the FTDI only for TX/RX/GND.

### 5.5 Assembly Order

1. Cut power. Never wire a live circuit.
2. Build the 5V and GND rails first (a small piece of perfboard or a screw terminal block works well).
3. Solder the 470 µF capacitor across the rails, watching polarity — the striped side is negative.
4. Connect ESP32-CAM 5V and GND.
5. Connect PIR VCC and GND.
6. Run the single signal wire: PIR `OUT` → ESP32-CAM `GPIO 13`.
7. Double-check with a multimeter in continuity mode that 5V and GND are **not** shorted before applying power.

---

## 6. Understanding the PIR Sensor

The HC-SR501 has two orange potentiometers and one jumper on its underside. Getting these right saves hours of frustration.

| Control | Function | Recommended setting |
|---|---|---|
| **Sensitivity (Sx)** pot | Detection range, roughly 3 m to 7 m | Start at ~50%, then tune |
| **Time delay (Tx)** pot | How long OUT stays HIGH after a trigger (3 s to 300 s) | Fully anti-clockwise (minimum, ~3 s) |
| **Trigger jumper** | `H` = repeatable, `L` = non-repeatable | **H (repeatable)** |

**Why these values:** we want the sensor to reset quickly so a second person walking past a few seconds later produces a fresh trigger. The software handles the cooldown between photos, not the sensor.

**Warm-up period:** after power-on the HC-SR501 needs **30–60 seconds** to calibrate to the ambient infrared level. It will fire randomly during this window. This is normal and is handled in the code with a startup delay.

**What causes false triggers:**
- Direct sunlight or headlights sweeping across the lens
- Hot air currents — never mount it above a heat source or facing an AC outdoor unit
- Moving vegetation warmed by the sun
- The ESP32's own heat if the sensor is mounted too close inside a sealed box

---

## 7. Setting Up the Telegram Bot

### Step 1 — Create the bot

1. Open Telegram and search for **@BotFather** (the one with the blue verified tick).
2. Send `/newbot`.
3. Give it a display name, e.g. `Home Security Cam`.
4. Give it a username ending in `bot`, e.g. `my_home_security_2026_bot`.
5. BotFather replies with a **token** that looks like:
   ```
   7123456789:AAHxyzABCdefGHIjklMNOpqrSTUvwxYZ123
   ```
6. Copy it. **Treat this token like a password** — anyone holding it can control your bot.

### Step 2 — Get your Chat ID

The bot needs to know *who* to send photos to.

1. Search for **@userinfobot** (or `@IDBot`) in Telegram.
2. Press Start.
3. It replies with your numeric ID, e.g. `1234567890`.

### Step 3 — Open the chat with your own bot

Search for your bot's username, open it, and press **Start**. A bot cannot message a user who has never initiated a conversation with it. Skip this and every send will silently fail with `chat not found`.

### Step 4 — Quick sanity check (optional but useful)

Paste this into a browser, substituting your values:

```
https://api.telegram.org/bot<YOUR_TOKEN>/sendMessage?chat_id=<YOUR_CHAT_ID>&text=Hello
```

If "Hello" arrives in Telegram, your token and chat ID are both correct and the problem — should one appear later — is in the hardware or the sketch, not the bot.

---

## 8. Software Setup (Arduino IDE)

### 8.1 Install the ESP32 board package

1. Open Arduino IDE → **File → Preferences**.
2. In *Additional Board Manager URLs*, paste:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager**, search `esp32`, install **esp32 by Espressif Systems**.

### 8.2 Install libraries

**Sketch → Include Library → Manage Libraries**, then install:

| Library | Author | Purpose |
|---|---|---|
| **UniversalTelegramBot** | Brian Lough | Telegram API wrapper |
| **ArduinoJson** | Benoit Blanchon | Required dependency (use v6.x) |

> ⚠️ ArduinoJson v7 changed its API. If you hit compile errors mentioning `StaticJsonDocument`, roll back to the latest **6.x** release in the Library Manager version dropdown.

### 8.3 Board settings

Select **Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM**, then set:

| Setting | Value |
|---|---|
| Board | AI Thinker ESP32-CAM |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| CPU Frequency | 240 MHz |
| Flash Frequency | 80 MHz |
| Flash Mode | QIO |
| Upload Speed | 115200 |
| Core Debug Level | None |
| Port | Whatever COM/ttyUSB your FTDI enumerates as |

> The **Huge APP** partition scheme is not optional. The default scheme does not leave enough application space for the camera driver plus the TLS stack, and you will get a `Sketch too big` error.

---

## 9. The Code

Create a new sketch, paste the following, and edit the four configuration lines at the top.

```cpp
/*
 * ESP32-CAM Home Security System
 * PIR motion detection -> photo capture -> Telegram delivery
 *
 * Board:      AI Thinker ESP32-CAM
 * Partition:  Huge APP (3MB No OTA / 1MB SPIFFS)
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ─────────── CONFIGURE THESE ───────────
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BOT_TOKEN     = "7123456789:AAHxyz...";   // from BotFather
const String CHAT_ID      = "1234567890";             // from @userinfobot
// ───────────────────────────────────────

#define PIR_PIN        13        // PIR OUT wired here
#define FLASH_LED_PIN   4        // onboard white LED
#define USE_FLASH      false     // set true for night-time capture
#define COOLDOWN_MS    15000UL   // minimum gap between photos
#define WARMUP_MS      45000UL   // PIR calibration time after boot

// ── AI-Thinker OV2640 camera pin map ──
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

WiFiClientSecure secured;
UniversalTelegramBot bot(BOT_TOKEN, secured);

unsigned long lastShot = 0;
unsigned long bootTime = 0;

// ──────────────────────────────────────────────────────────
//  Camera initialisation
// ──────────────────────────────────────────────────────────
bool startCamera() {
  camera_config_t cfg;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM;   cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM;   cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM;   cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM;   cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk  = XCLK_GPIO_NUM;
  cfg.pin_pclk  = PCLK_GPIO_NUM;
  cfg.pin_vsync = VSYNC_GPIO_NUM;
  cfg.pin_href  = HREF_GPIO_NUM;
  cfg.pin_sccb_sda = SIOD_GPIO_NUM;
  cfg.pin_sccb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn  = PWDN_GPIO_NUM;
  cfg.pin_reset = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    cfg.frame_size   = FRAMESIZE_SVGA;   // 800x600
    cfg.jpeg_quality = 10;               // lower number = better quality
    cfg.fb_count     = 2;
  } else {
    cfg.frame_size   = FRAMESIZE_CIF;    // 400x296
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
  }

  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  s->set_brightness(s, 1);
  s->set_saturation(s, 0);
  // If your camera is mounted upside down, uncomment:
  // s->set_vflip(s, 1);
  // s->set_hmirror(s, 1);
  return true;
}

// ──────────────────────────────────────────────────────────
//  Upload one JPEG to Telegram as multipart/form-data
// ──────────────────────────────────────────────────────────
bool sendPhoto() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Capture failed");
    return false;
  }

  Serial.printf("Captured %u bytes, uploading...\n", fb->len);

  if (!secured.connect("api.telegram.org", 443)) {
    Serial.println("TLS connection failed");
    esp_camera_fb_return(fb);
    return false;
  }

  String boundary = "esp32camboundary";
  String head = "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n"
                + CHAT_ID + "\r\n"
                "--" + boundary + "\r\n"
                "Content-Disposition: form-data; name=\"photo\"; "
                "filename=\"motion.jpg\"\r\n"
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();

  secured.println("POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1");
  secured.println("Host: api.telegram.org");
  secured.println("Content-Length: " + String(totalLen));
  secured.println("Content-Type: multipart/form-data; boundary=" + boundary);
  secured.println();
  secured.print(head);

  // stream the image in 1 KB chunks
  uint8_t* buf = fb->buf;
  size_t   len = fb->len;
  for (size_t i = 0; i < len; i += 1024) {
    size_t chunk = (len - i >= 1024) ? 1024 : (len % 1024);
    secured.write(buf + i, chunk);
  }
  secured.print(tail);
  esp_camera_fb_return(fb);

  // read the response (with timeout)
  String response = "";
  unsigned long start = millis();
  while (millis() - start < 10000) {
    while (secured.available()) {
      response += (char)secured.read();
      start = millis();
    }
    if (response.length() && !secured.connected()) break;
  }
  secured.stop();

  bool ok = response.indexOf("\"ok\":true") > 0;
  Serial.println(ok ? "Sent." : "Telegram rejected the upload.");
  return ok;
}

// ──────────────────────────────────────────────────────────
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);   // disable brownout detector

  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println("\n\nESP32-CAM Security System booting...");

  pinMode(PIR_PIN, INPUT);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!startCamera()) {
    delay(3000);
    ESP.restart();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP address: " + WiFi.localIP().toString());

  secured.setInsecure();   // skip root-CA validation (see note below)

  bot.sendMessage(CHAT_ID, "🟢 Security system online.", "");
  bootTime = millis();
  Serial.println("PIR warming up (45s)...");
}

void loop() {
  // ignore the sensor while it calibrates
  if (millis() - bootTime < WARMUP_MS) {
    delay(100);
    return;
  }

  if (digitalRead(PIR_PIN) == HIGH && millis() - lastShot > COOLDOWN_MS) {
    Serial.println("Motion detected!");

    if (USE_FLASH) { digitalWrite(FLASH_LED_PIN, HIGH); delay(120); }
    esp_camera_fb_return(esp_camera_fb_get());   // discard stale frame
    bool ok = sendPhoto();
    if (USE_FLASH) digitalWrite(FLASH_LED_PIN, LOW);

    if (ok) lastShot = millis();
  }

  // auto-reconnect if Wi-Fi drops
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000);
  }

  delay(50);
}
```

### Notes on the code

- **`secured.setInsecure()`** skips TLS certificate verification. It keeps the connection encrypted but does not verify that the server really is Telegram. For a hobby project on your own network this is the pragmatic choice, since Telegram rotates its certificates and a pinned root CA will eventually expire and silently break the device. If you want strict verification, use `secured.setCACert(TELEGRAM_CERTIFICATE_ROOT)` from the UniversalTelegramBot library and be prepared to reflash when it changes.
- **The discarded frame** before capture matters. The OV2640 keeps an old buffer; without throwing one away you frequently send a photo of the scene from several seconds ago.
- **`COOLDOWN_MS`** is your main defence against spam. At 15 seconds a continuously busy street yields at most 240 photos per hour.
- **Brownout disable** stops the board resetting when the Wi-Fi radio spikes current draw. It is a workaround, not a fix — a proper 2A supply and the capacitor are the real fix.

---

## 10. Uploading to the ESP32-CAM

1. Wire the FTDI as in [section 5.4](#54-wiring-table--programming-only-ftdi), including the **GPIO 0 → GND** jumper.
2. Plug the FTDI into your computer.
3. Press the **RST** button on the back of the ESP32-CAM.
4. In Arduino IDE, select the right port and hit **Upload**.
5. When you see `Connecting........___`, the board is waiting — if it times out, press RST again while that message is showing.
6. On `Hard resetting via RTS pin...`, the upload is done.
7. **Remove the GPIO 0 jumper.**
8. Press RST once more. The sketch now runs.
9. Open **Serial Monitor at 115200 baud** to watch the boot log.

> If you bought the **ESP32-CAM-MB** shield, skip the jumper entirely — press and hold its `IO0` button, tap `RST`, release `IO0`, then upload.

---

## 11. First Run and Testing

Watch the Serial Monitor. A healthy boot looks like this:

```
ESP32-CAM Security System booting...
Connecting to Wi-Fi.....
IP address: 192.168.1.47
PIR warming up (45s)...
```

And a Telegram message reading **🟢 Security system online.** should arrive.

Then:

1. Wait out the 45-second warm-up without moving in front of the sensor.
2. Walk across the sensor's field of view.
3. Serial should print `Motion detected!` → `Captured NNNNN bytes, uploading...` → `Sent.`
4. The photo appears in Telegram.

**Calibration walk:** stand at the furthest point you want covered and walk across. If nothing triggers, turn the sensitivity pot clockwise a little. If your own pet sets it off from ten metres away, turn it back. Adjust in small increments and re-test — the pots are quite sensitive.

---

## 12. Building the Enclosure

I used a plastic box that was already lying around at home. Nothing fancy is required.

1. **Position the components on the lid first** and mark with a pen where the camera lens and the PIR dome need to poke through.
2. **Camera hole:** the OV2640 lens barrel is about 8 mm. Drill slightly undersize and widen with a round file so the barrel is a snug press fit — that alone holds the camera steady.
3. **PIR hole:** the white Fresnel dome is about 23 mm. Use a step drill or a hot soldering-iron tip traced around a marked circle, then clean up the edge.
4. **Mount the boards** with hot glue on the corners, or better, small standoffs so air can flow underneath. Do not glue over the ESP32 shield — it needs to radiate heat.
5. **Keep the PIR physically distant from the ESP32.** The ESP32 runs warm, and a PIR sensor mounted right beside a warm chip inside a closed box will trigger on its own heat.
6. **Strain-relieve the power cable** with a knot inside the box or a cable gland so a tug does not rip your solder joints off.
7. **Route the antenna** away from metal. If you have poor Wi-Fi, desolder the tiny 0-ohm resistor/jumper near the antenna label and switch to an external IPEX antenna.

---

## 13. Heat Management

This was the single biggest problem in the build. **The ESP32-CAM gets genuinely hot** — the Wi-Fi radio and the camera together can push the module past 60 °C, and in a sealed plastic box with no airflow it kept climbing until the plastic around the board began to soften and deform.

**How it was solved:**

- **Drilled multiple small ventilation holes** across the box — a grid of ~3 mm holes on the underside and along the rear face. Convection does the rest: cool air in from the bottom, warm air out of the top.
- Keep the holes on the **underside and back**, never the top face, so rain and dust cannot drop straight in.
- **Small holes, many of them** beat one big hole: same airflow, far less chance of insects getting in.

**Additional measures worth taking:**

| Measure | Effect |
|---|---|
| Stick a 8×8×5 mm aluminium heatsink on the ESP32 shield with thermal tape | Drops module temperature noticeably |
| Mount the board vertically | Convection works along the board, not against it |
| Lower the frame size to `FRAMESIZE_VGA` | Shorter radio-on time per photo |
| Increase `COOLDOWN_MS` | Fewer captures, less sustained load |
| Avoid direct sunlight on the box | A dark box in sun can hit 50 °C before the electronics add anything |

Do not seal the box airtight "for weatherproofing". A vented box in a sheltered location outlasts a sealed box that cooks itself.

---

## 14. Installation and Placement

- **Shelter it.** I mounted mine in a spot already covered by a metal sheet, so rain is never an issue. If you have no covered spot, add a small overhang above the box.
- **Height:** 2 to 2.5 metres, angled slightly downward. High enough to be out of reach, low enough to capture faces rather than the tops of heads.
- **Avoid pointing at:** the road with passing car headlights, a west-facing view into the setting sun, or anything that moves in the wind.
- **Wi-Fi check:** the ESP32 only does **2.4 GHz**. It will not see your 5 GHz network. If your router broadcasts both under one name, you may need a separate 2.4 GHz SSID.
- **Power:** run the 5V from indoors if possible rather than putting an adapter outside.

---

## 15. Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `Brownout detector was triggered` in a boot loop | Underpowered supply | Use a 2A adapter, add the 470 µF cap, use thicker/shorter wires |
| `Camera init failed: 0x105` | Ribbon cable not seated | Open the connector latch, reseat the ribbon fully, close the latch |
| `Sketch too big` | Wrong partition scheme | Tools → Partition Scheme → Huge APP |
| Upload fails: `Failed to connect... Timed out waiting for packet header` | Not in flash mode | GPIO 0 must be grounded; press RST just as upload begins |
| Connects to Wi-Fi but never sends | Bot chat not started | Open your bot in Telegram and press Start |
| `chat not found` | Wrong chat ID | Re-check with @userinfobot; group IDs begin with `-` |
| Photos arrive but are green / striped / garbled | Power sag during capture, or faulty ribbon | Better PSU; reduce frame size; reseat ribbon |
| Photo shows an old scene | Stale frame buffer | Keep the discard-a-frame line in `loop()` |
| Constant false triggers | PIR still warming up, or heat/sunlight | Wait 60 s; move PIR away from the ESP32; shade the lens |
| No triggers at all | Sensitivity too low, or wrong pin | Turn Sx pot clockwise; confirm OUT → GPIO 13; check common ground |
| Works for a while then goes silent | Wi-Fi dropped, or overheating | The reconnect block handles drops; check ventilation |
| Cannot find the board's COM port | Missing USB driver | Install CP2102 or CH340 drivers for your OS |

**Useful debugging trick:** comment out the camera entirely and just `Serial.println(digitalRead(PIR_PIN))` in a loop. If the PIR is working you will see the value flip to 1 when you wave at it. That isolates sensor problems from camera problems in about thirty seconds.

---

## 16. Tuning and Optimisation

**Image quality vs speed**

| `frame_size` | Resolution | Typical size | Upload time |
|---|---|---|---|
| `FRAMESIZE_QVGA` | 320×240 | ~8 KB | ~1 s |
| `FRAMESIZE_VGA` | 640×480 | ~25 KB | ~2 s |
| `FRAMESIZE_SVGA` | 800×600 | ~40 KB | ~3 s |
| `FRAMESIZE_UXGA` | 1600×1200 | ~120 KB | ~7 s |

`jpeg_quality` runs 0–63 where **lower is better quality and a bigger file**. Values below 10 on UXGA frequently exhaust memory — 10 to 12 is the sweet spot.

**Other ideas**

- Send a burst of 3 photos a second apart to catch a face, not just a back.
- Add `bot.sendMessage()` with a timestamp alongside the photo (requires NTP).
- Save a copy to the microSD card as an offline backup in case Wi-Fi is down.
- Add a `/photo` command handler so you can request a snapshot on demand from Telegram.
- Use deep sleep with the PIR wired to an RTC-capable pin if you want to run this on a battery. Note GPIO 13 is RTC-capable, so `esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 1)` works — but a battery will still only last days, not months, and mains power is far more practical for a fixed installation.

---

## 17. Results

The system has been running for **months continuously**, sending **thousands of photos per day**, with no crashes, no manual restarts and no degradation. Since Telegram handles delivery, alerts arrive just as reliably when I am on a different continent as when I am in the next room. Rain has never been an issue because the unit sits under an existing metal sheet.

The only real engineering problem in the whole build was thermal, and the ventilation holes solved it permanently.

---

## 18. Future Improvements

- [ ] Person detection to filter out cats, birds and swaying branches
- [ ] Night vision using an IR-sensitive camera module and an IR LED array
- [ ] Two-way control from Telegram (`/arm`, `/disarm`, `/status`, `/photo`)
- [ ] Local SD backup with automatic rotation
- [ ] Battery + solar for locations without mains
- [ ] Short video clips instead of stills
- [ ] Multi-camera setup with one bot serving several nodes
- [ ] OTA firmware updates so the box never has to come down again

---

## 19. Privacy and Legal Note

A camera pointed at a public street records people who have not consented to it. Rules on this vary a great deal between countries and even between states or municipalities — some require signage, some restrict how long footage may be kept, some prohibit covering public pavement at all.

Before you install this permanently, it is worth checking what applies where you live. Practical good habits regardless of jurisdiction: aim the camera at your own property rather than across a neighbour's windows, do not share captured images publicly, and keep your bot token private so nobody else can pull images from your device.

This project is shared for learning and for protecting your own property. What you do with it is your responsibility.

---

## 20. Credits

- **ESP32-CAM AI-Thinker** — Espressif / AI-Thinker
- **UniversalTelegramBot** — Brian Lough
- **ArduinoJson** — Benoit Blanchon
- **Telegram Bot API** — Telegram

Built, broken, melted slightly, and fixed by hand. 🔧

---

### License

MIT — do what you like with it, no warranty offered.
