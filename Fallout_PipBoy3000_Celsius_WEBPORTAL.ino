// TFT_eSPI_memory
//
// Animated GIF on TFT with Fallout PipBoy interface
// Fallout-themed WiFiManager captive portal + TFT debug boot sequence
//
// Adapted by Bodmer for the TFT_eSPI Arduino library
// Enhanced by Jéjé l'Ingé - jeje-linge.fr

//========================USEFUL VARIABLES=============================
int UTC = 2;                    // Set your time zone ex: france = UTC+2
uint16_t notification_volume = 30;
//=====================================================================
bool res;

// Load GIF library
#include <AnimatedGIF.h>
AnimatedGIF gif;

#include ".\images/INIT_2.h"
#include ".\images/STAT.h"
#include ".\images/RADIO.h"
#include ".\images/DATA_1.h"
#include ".\images/TIME.h"
#include ".\images/Bottom_layer_2.h"
#include ".\images/Date.h"
#include ".\images/INV.h"
#include ".\images/temperatureTemp_hum.h"
#include ".\images/RADIATION.h"
#include ".\images/Morning.h"
#include ".\images/Afternoon.h"
#include ".\images/temperatureTemp_hum_F.h"

#define INIT_2 INIT
#define TIME TIME
#define STAT STAT
#define DATA_1 DATA_1
#define INV INV

#define IN_STAT 25
#define IN_INV 26
#define IN_DATA 27
#define IN_TIME 32
#define IN_RADIO 33

#define REPEAT_CAL false
#define Light_green  0x35C2
#define Dark_green   0x0261
#define Time_color   0x04C0
#define Amber_warn   0xFDA0  // Orange/amber for warnings
#define Red_error    0xF800  // Red for errors

#include "WiFiManager.h"
#include "NTPClient.h"
#include "DFRobotDFPlayerMini.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_SHT31.h"

const byte RXD2 = 16;
const byte TXD2 = 17;

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
#define FPSerial Serial1
TFT_eSPI tft = TFT_eSPI();
Adafruit_SHT31 sht31 = Adafruit_SHT31();

int i = 0;
int a = 0;
uint16_t x = 0, y = 0;
int interupt = 1;
float t_far = 0;
int hh = 0;
int mm = 0;
int ss = 0;
int flag = 0;
int prev_hour = 0;
String localip;
bool enableHeater = false;
uint8_t loopCnt = 0;
const long utcOffsetInSeconds = 3600;
uint32_t targetTime = 0;
static uint8_t conv2d(const char* p);

// Track subsystem status for debug
bool dfPlayerOK = false;
bool sensorOK = false;
bool wifiOK = false;
bool ntpOK = false;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds * UTC);

byte omm = 99, oss = 99;
byte xcolon = 0, xsecs = 0;
unsigned int colour = 0;

// ============================================================
//  FALLOUT-THEMED BOOT TERMINAL ON TFT
// ============================================================

int bootLine = 0;           // Current Y position for boot messages
const int BOOT_LINE_H = 22; // Line height in pixels
const int BOOT_X = 8;       // Left margin
const int BOOT_START_Y = 5; // Top margin

void bootClear() {
  tft.fillScreen(TFT_BLACK);
  bootLine = BOOT_START_Y;
}

// Print a boot line with status indicator
// status: 0 = info (green), 1 = OK (green), 2 = WARN (amber), 3 = ERROR (red), 4 = header
void bootPrint(const char* msg, uint8_t status = 0) {
  if (bootLine > 290) {  // Screen full, scroll up visually
    bootClear();
  }

  uint16_t color;
  const char* prefix;
  switch (status) {
    case 1:  color = Light_green; prefix = "[  OK  ] "; break;
    case 2:  color = Amber_warn;  prefix = "[ WARN ] "; break;
    case 3:  color = Red_error;   prefix = "[ FAIL ] "; break;
    case 4:  // Header / title
      tft.setTextColor(Light_green, TFT_BLACK);
      tft.drawString(msg, BOOT_X, bootLine, 2);
      bootLine += BOOT_LINE_H + 4;
      return;
    default: color = Dark_green;  prefix = "  > "; break;
  }

  tft.setTextColor(color, TFT_BLACK);
  String line = String(prefix) + String(msg);
  tft.drawString(line, BOOT_X, bootLine, 2);
  bootLine += BOOT_LINE_H;

  // Also print to Serial
  Serial.println(line);
}

// Draws the decorative header
void bootHeader() {
  bootClear();
  tft.setTextColor(Light_green, TFT_BLACK);

  // PipBoy terminal header
  tft.drawString("ROBCO INDUSTRIES (TM) TERMLINK", BOOT_X, BOOT_START_Y, 2);
  bootLine = BOOT_START_Y + BOOT_LINE_H;
  tft.drawString("PIPBOY 3000 MARK IV OS v4.1.7", BOOT_X, bootLine, 2);
  bootLine += BOOT_LINE_H;

  // Decorative separator
  tft.drawString("================================", BOOT_X, bootLine, 2);
  bootLine += BOOT_LINE_H + 4;
}

// Show a blinking cursor effect (brief)
void bootCursor(int count = 3) {
  for (int i = 0; i < count; i++) {
    tft.fillRect(BOOT_X, bootLine, 10, 14, Light_green);
    delay(150);
    tft.fillRect(BOOT_X, bootLine, 10, 14, TFT_BLACK);
    delay(100);
  }
}

// Show final status summary before launching main app
void bootSummary() {
  bootLine += 6;
  tft.drawLine(BOOT_X, bootLine, 470, bootLine, Dark_green);
  bootLine += 8;

  tft.setTextColor(Light_green, TFT_BLACK);
  String summary = "SYS: WiFi[";
  summary += wifiOK   ? "OK" : "!!";
  summary += "] NTP[";
  summary += ntpOK    ? "OK" : "!!";
  summary += "] AUD[";
  summary += dfPlayerOK ? "OK" : "!!";
  summary += "] SENS[";
  summary += sensorOK ? "OK" : "!!";
  summary += "]";
  tft.drawString(summary, BOOT_X, bootLine, 2);
  bootLine += BOOT_LINE_H;

  if (wifiOK && dfPlayerOK && sensorOK) {
    bootPrint("ALL SYSTEMS NOMINAL", 1);
  } else {
    bootPrint("BOOT WITH WARNINGS - CHECK LOG", 2);
  }
  delay(1500);
}

// ============================================================
//  FALLOUT CSS FOR WIFIMANAGER CAPTIVE PORTAL
// ============================================================

const char PIPBOY_CSS[] PROGMEM = R"rawliteral(
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap');

  :root {
    --pip-green: #30ff50;
    --pip-dark: #0a3c12;
    --pip-bg: #0b1a0e;
    --pip-border: #1a5c2a;
    --pip-glow: rgba(48, 255, 80, 0.15);
    --pip-dim: #186828;
  }

  * { box-sizing: border-box; }

  body {
    background-color: var(--pip-bg) !important;
    color: var(--pip-green) !important;
    font-family: 'Share Tech Mono', 'Courier New', monospace !important;
    margin: 0; padding: 20px;
    min-height: 100vh;
    background-image:
      repeating-linear-gradient(
        0deg,
        transparent,
        transparent 2px,
        rgba(0,0,0,0.15) 2px,
        rgba(0,0,0,0.15) 4px
      );
    animation: flicker 0.15s infinite alternate;
  }

  @keyframes flicker {
    0%   { opacity: 0.97; }
    100% { opacity: 1; }
  }

  /* Scanline overlay */
  body::after {
    content: '';
    position: fixed; top: 0; left: 0;
    width: 100%; height: 100%;
    pointer-events: none;
    background: repeating-linear-gradient(
      0deg,
      rgba(0,0,0,0) 0px,
      rgba(0,0,0,0) 1px,
      rgba(0,0,0,0.1) 1px,
      rgba(0,0,0,0.1) 2px
    );
    z-index: 9999;
  }

  .wrap {
    max-width: 480px !important;
    margin: 0 auto;
    padding: 20px;
    border: 1px solid var(--pip-border);
    background: rgba(10, 60, 18, 0.3);
    box-shadow: 0 0 30px var(--pip-glow), inset 0 0 30px rgba(0,0,0,0.5);
  }

  /* Header area */
  div[style*='text-align:left'], .msg {
    border-bottom: 1px solid var(--pip-dim);
    padding-bottom: 12px;
    margin-bottom: 16px;
  }

  h1, h2, h3 {
    color: var(--pip-green) !important;
    text-shadow: 0 0 10px var(--pip-green), 0 0 20px rgba(48,255,80,0.3);
    font-weight: normal !important;
    letter-spacing: 2px;
    text-transform: uppercase;
  }

  /* Input fields */
  input[type="text"],
  input[type="password"],
  select {
    background: var(--pip-dark) !important;
    color: var(--pip-green) !important;
    border: 1px solid var(--pip-dim) !important;
    font-family: 'Share Tech Mono', monospace !important;
    font-size: 16px !important;
    padding: 10px 12px !important;
    width: 100% !important;
    outline: none !important;
    border-radius: 0 !important;
    transition: border-color 0.2s, box-shadow 0.2s;
  }

  input:focus, select:focus {
    border-color: var(--pip-green) !important;
    box-shadow: 0 0 8px var(--pip-glow) !important;
  }

  input::placeholder {
    color: var(--pip-dim) !important;
    opacity: 1;
  }

  /* Buttons */
  button, input[type="submit"], .D {
    background: transparent !important;
    color: var(--pip-green) !important;
    border: 1px solid var(--pip-green) !important;
    font-family: 'Share Tech Mono', monospace !important;
    font-size: 15px !important;
    padding: 12px 20px !important;
    cursor: pointer;
    text-transform: uppercase;
    letter-spacing: 2px;
    width: 100% !important;
    margin: 6px 0 !important;
    transition: all 0.15s;
    border-radius: 0 !important;
    text-decoration: none !important;
    display: block !important;
    text-align: center !important;
  }

  button:hover, input[type="submit"]:hover, .D:hover {
    background: var(--pip-green) !important;
    color: var(--pip-bg) !important;
    box-shadow: 0 0 15px var(--pip-glow);
    text-shadow: none;
  }

  button:active, input[type="submit"]:active {
    transform: scale(0.98);
  }

  /* Links */
  a {
    color: var(--pip-green) !important;
    text-decoration: none !important;
  }
  a:hover { text-decoration: underline !important; }

  /* Labels */
  label { color: var(--pip-dim) !important; font-size: 13px; text-transform: uppercase; letter-spacing: 1px; }

  /* Quality bars */
  .q {
    color: var(--pip-dim) !important;
  }

  /* Footer / info text */
  .c, .msg, small {
    color: var(--pip-dim) !important;
    font-size: 12px;
  }

  /* Custom branding header */
  #pipboy-brand {
    text-align: center;
    padding: 15px 0 10px 0;
    border-bottom: 2px solid var(--pip-dim);
    margin-bottom: 16px;
  }
  #pipboy-brand h2 {
    margin: 0 0 4px 0;
    font-size: 20px;
  }
  #pipboy-brand .sub {
    color: var(--pip-dim);
    font-size: 11px;
    letter-spacing: 3px;
  }
</style>

<div id="pipboy-brand">
  <h2>// PIPBOY 3000 //</h2>
  <div class="sub">ROBCO INDUSTRIES UNIFIED OS</div>
  <div class="sub">NETWORK CONFIGURATION MODULE</div>
</div>
)rawliteral";

// ============================================================
//  SETUP
// ============================================================

void setup() {

  pinMode(IN_RADIO, INPUT_PULLUP);
  pinMode(IN_STAT, INPUT_PULLUP);
  pinMode(IN_DATA, INPUT_PULLUP);
  pinMode(IN_INV, INPUT_PULLUP);
  pinMode(IN_TIME, INPUT_PULLUP);

  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);

  // ---- BOOT SEQUENCE ----
  bootHeader();
  delay(400);

  bootPrint("Initializing subsystems...");
  bootCursor(2);
  delay(200);

  // ---- WiFi Setup ----
  bootPrint("Loading WiFi module...");

  WiFiManager manager;
  manager.setTimeout(120);

  // Inject Fallout CSS into captive portal
  manager.setCustomHeadElement(PIPBOY_CSS);
  manager.setTitle("PIPBOY 3000");
  manager.setClass("invert");   // dark theme base

  // Show step-by-step portal instructions on TFT
  bootPrint("Searching for saved network...");
  bootCursor(2);
  bootLine += 4;
  bootPrint("WIFI SETUP INSTRUCTIONS:", 4);
  bootPrint("1. Open WiFi on your phone", 0);
  bootPrint("2. Connect to: PIPBOY3000", 0);
  bootPrint("3. Password:   password", 0);
  bootPrint("4. Open 192.168.4.1 in browser", 0);
  bootPrint("5. Select your WiFi network", 0);
  bootLine += 4;
  bootPrint("Waiting for connection...", 2);

  res = manager.autoConnect("PIPBOY3000", "password");

  if (!res) {
    wifiOK = false;
    bootPrint("WiFi connection FAILED", 3);
    bootPrint("Timeout after 120s", 3);
    bootLine += 6;
    bootPrint("Check SSID and PASSWORD", 2);
    bootPrint("Rebooting in 6 seconds...", 2);
    delay(6000);
    bootPrint(">>> ESP.restart()", 0);
    delay(1000);
    ESP.restart();
  }

  wifiOK = true;
  localip = WiFi.localIP().toString();
  bootPrint("WiFi connected", 1);

  String ipMsg = "IP: " + localip;
  bootPrint(ipMsg.c_str(), 0);
  delay(300);

  // ---- NTP Time Client ----
  bootPrint("Starting NTP client...");
  timeClient.begin();
  timeClient.update();

  if (timeClient.getEpochTime() > 1000000) {
    ntpOK = true;
    bootPrint("NTP time synced", 1);
  } else {
    ntpOK = false;
    bootPrint("NTP sync failed - retrying...", 2);
    delay(2000);
    timeClient.update();
    if (timeClient.getEpochTime() > 1000000) {
      ntpOK = true;
      bootPrint("NTP time synced (retry)", 1);
    } else {
      bootPrint("NTP unavailable - time may drift", 2);
    }
  }
  delay(200);

  // ---- DFPlayer Mini ----
  bootPrint("Init DFPlayer serial...");
  FPSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);

  bootPrint("Connecting DFPlayer Mini...");
  if (!myDFPlayer.begin(FPSerial, true, true)) {
    dfPlayerOK = false;
    bootPrint("DFPlayer OFFLINE", 3);
    bootPrint("1. Check wiring RX/TX", 2);
    bootPrint("2. Insert SD card (FAT32)", 2);
    bootPrint("Audio disabled - continuing...", 2);
    delay(2000);
    // NOTE: Graceful degradation instead of blocking forever
  } else {
    dfPlayerOK = true;
    myDFPlayer.volume(notification_volume);
    myDFPlayer.setTimeOut(500);
    bootPrint("DFPlayer online", 1);

    String volMsg = "Volume: " + String(notification_volume) + "/30";
    bootPrint(volMsg.c_str(), 0);
  }
  delay(200);

  // ---- SHT31 Temperature Sensor ----
  bootPrint("Scanning I2C for SHT31...");
  if (!sht31.begin(0x44)) {
    sensorOK = false;
    bootPrint("SHT31 NOT FOUND (0x44)", 3);
    bootPrint("Check I2C wiring SDA/SCL", 2);
    bootPrint("Temp/humidity disabled", 2);
    delay(2000);
    // Graceful degradation instead of blocking
  } else {
    sensorOK = true;
    bootPrint("SHT31 sensor online", 1);

    if (sht31.isHeaterEnabled()) {
      bootPrint("Heater: ENABLED", 0);
    } else {
      bootPrint("Heater: DISABLED", 0);
    }
  }
  delay(200);

  // ---- Boot Summary ----
  bootSummary();

  // ---- GIF Engine ----
  gif.begin(BIG_ENDIAN_PIXELS);

  // ---- Play startup sound & animation ----
  if (dfPlayerOK) {
    myDFPlayer.playMp3Folder(1);
  }

  delay(500);

  if (gif.open((uint8_t *)INIT, sizeof(INIT), GIFDraw)) {
    tft.startWrite();
    while (gif.playFrame(true, NULL)) {
      yield();
    }
    gif.close();
    tft.endWrite();
  }
}

// ============================================================
//  TFT ERROR OVERLAY - Show errors during runtime
// ============================================================

// Display a temporary error/warning toast at bottom of screen
void tftToast(const char* msg, uint8_t level = 2) {
  uint16_t color = (level == 3) ? Red_error : Amber_warn;
  uint16_t bgColor = TFT_BLACK;

  // Draw toast bar at bottom
  tft.fillRect(0, 290, 480, 30, bgColor);
  tft.drawRect(0, 290, 480, 30, color);
  tft.setTextColor(color, bgColor);
  tft.drawString(msg, 8, 296, 2);
}

// Clear the toast area
void tftToastClear() {
  tft.fillRect(0, 290, 480, 30, TFT_BLACK);
}

// Full-screen error with Fallout terminal style
void tftErrorScreen(const char* title, const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
  tft.fillScreen(TFT_BLACK);

  // Red border
  tft.drawRect(2, 2, 476, 316, Red_error);
  tft.drawRect(3, 3, 474, 314, Red_error);

  // Header
  tft.setTextColor(Red_error, TFT_BLACK);
  tft.drawString("!!! SYSTEM ERROR !!!", 120, 15, 2);

  // Separator
  tft.drawLine(10, 40, 470, 40, Red_error);

  // Error title
  tft.setTextColor(Amber_warn, TFT_BLACK);
  tft.drawString(title, 20, 55, 4);

  // Details
  tft.setTextColor(Light_green, TFT_BLACK);
  int y = 100;
  if (line1) { tft.drawString(line1, 20, y, 2); y += 25; }
  if (line2) { tft.drawString(line2, 20, y, 2); y += 25; }
  if (line3) { tft.drawString(line3, 20, y, 2); y += 25; }

  // Footer
  tft.setTextColor(Dark_green, TFT_BLACK);
  tft.drawString("ROBCO INDUSTRIES TERMLINK", 130, 280, 2);
}

// ============================================================
//  PLAY SOUND (safe - checks dfPlayerOK)
// ============================================================

void playSound(int folder_min, int folder_max) {
  if (dfPlayerOK) {
    myDFPlayer.playMp3Folder(random(folder_min, folder_max));
  }
}

// ============================================================
//  MAIN LOOP
// ============================================================

void loop() {

  timeClient.update();

  // Debug: print time to Serial
  Serial.print("Time: ");
  Serial.println(timeClient.getFormattedTime());

  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int currentYear = ptm->tm_year + 1900;
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;

  // Daylight saving time adjustment
  if ((currentMonth * 30 + monthDay) >= 121 && (currentMonth * 30 + monthDay) < 331) {
    timeClient.setTimeOffset(utcOffsetInSeconds * UTC);       // Summer
  } else {
    timeClient.setTimeOffset((utcOffsetInSeconds * UTC) - 3600); // Winter
  }

  // ---- STAT Button ----
  if (digitalRead(IN_STAT) == false) {
    flag = 1;
    playSound(2, 5);
    while (digitalRead(IN_STAT) == false) {
      if (gif.open((uint8_t *)STAT, sizeof(STAT), GIFDraw)) {
        tft.startWrite();
        while (gif.playFrame(true, NULL)) { yield(); }
        gif.close();
        tft.endWrite();
      }
    }
  }

  // ---- INV Button ----
  if (digitalRead(IN_INV) == false) {
    flag = 1;
    playSound(2, 5);
    while (digitalRead(IN_INV) == false) {
      if (gif.open((uint8_t *)INV, sizeof(INV), GIFDraw)) {
        tft.startWrite();
        while (gif.playFrame(true, NULL)) { yield(); }
        gif.close();
        tft.endWrite();
      }
    }
  }

  // ---- DATA Button (Temperature/Humidity) ----
  if (digitalRead(IN_DATA) == false) {
    flag = 1;
    playSound(2, 5);
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);
    tft.drawBitmap(35, 80, temperatureTemp_humTemp_hum_2, 408, 29, Light_green);
    tft.drawBitmap(200, 200, RadiationRadiation, 62, 61, Light_green);

    while (digitalRead(IN_DATA) == false) {

      if (!sensorOK) {
        // Show error toast if sensor is offline
        tftToast("SENSOR OFFLINE - NO DATA", 3);
      } else {
        float t = sht31.readTemperature();
        float h = sht31.readHumidity();

        if (isnan(t) || isnan(h)) {
          tftToast("SENSOR READ ERROR", 3);
        } else {
          tftToastClear(); // Clear any previous error
          tft.setTextColor(Time_color, TFT_BLACK);
          t_far = (t * 1.8) + 32;
          tft.drawFloat(t, 2, 60, 135, 7);
          tft.drawFloat(h, 2, 258, 135, 7);
        }
      }

      if (gif.open((uint8_t *)DATA_1, sizeof(DATA_1), GIFDraw)) {
        tft.startWrite();
        while (gif.playFrame(true, NULL)) { yield(); }
        gif.close();
        tft.endWrite();
      }
    }
  }

  // ---- TIME Button ----
  if (digitalRead(IN_TIME) == false) {
    playSound(2, 5);
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);

    if (!ntpOK) {
      tftToast("NTP OFFLINE - TIME MAY BE WRONG", 2);
    }

    while (digitalRead(IN_TIME) == false) {
      if (gif.open((uint8_t *)TIME, sizeof(TIME), GIFDraw)) {
        tft.startWrite();
        while (gif.playFrame(true, NULL)) { yield(); }
        gif.close();
        tft.endWrite();
      }
      show_hour();
    }
  }

  // ---- RADIO Button ----
  if (digitalRead(IN_RADIO) == false) {
    flag = 1;

    if (dfPlayerOK) {
      myDFPlayer.playMp3Folder(random(5, 10));
    } else {
      // Show error if audio is offline
      tft.fillScreen(TFT_BLACK);
      tftErrorScreen(
        "AUDIO MODULE",
        "DFPlayer Mini is not responding.",
        "Check SD card and wiring.",
        "Radio function unavailable."
      );
      delay(3000);
    }

    delay(500);
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);

    if (!dfPlayerOK) {
      tftToast("AUDIO OFFLINE - NO SOUND", 3);
    }

    while (digitalRead(IN_RADIO) == false) {
      if (gif.open((uint8_t *)RADIO, sizeof(RADIO), GIFDraw)) {
        tft.startWrite();
        while (gif.playFrame(true, NULL)) { yield(); }
        gif.close();
        tft.endWrite();
      }
    }
  }

  // ---- Periodic WiFi check (every loop) ----
  if (WiFi.status() != WL_CONNECTED && wifiOK) {
    wifiOK = false;
    Serial.println("[WARN] WiFi connection lost!");
  }
  if (WiFi.status() == WL_CONNECTED && !wifiOK) {
    wifiOK = true;
    Serial.println("[OK] WiFi reconnected.");
  }
}

// ============================================================
//  SHOW HOUR
// ============================================================

void show_hour() {
  tft.setTextSize(2);
  mm = timeClient.getMinutes();
  ss = timeClient.getSeconds();

  if (timeClient.getHours() == 0) {
    hh = 12;
  } else if (timeClient.getHours() == 12) {
    hh = timeClient.getHours();
  } else if (timeClient.getHours() >= 13) {
    hh = timeClient.getHours() - 12;
  } else {
    hh = timeClient.getHours();
  }

  if (timeClient.getHours() != prev_hour) {
    tft.fillRect(140, 210, 200, 50, TFT_BLACK);
  }

  if (timeClient.getHours() < 12 && timeClient.getHours() > 0) {
    tft.drawBitmap(150, 220, MorningMorning, 170, 29, Light_green);
  } else {
    tft.drawBitmap(150, 220, afternoonAfternoon, 170, 29, Light_green);
  }

  int xpos = 85;
  int ypos = 90;
  int ysecs = ypos + 24;

  if (omm != mm || flag == 1) {
    omm = mm;
    tft.setTextColor(Time_color, TFT_BLACK);
    if (hh < 10) xpos += tft.drawChar('0', xpos, ypos, 7);
    xpos += tft.drawNumber(hh, xpos, ypos, 7);
    xcolon = xpos;
    xpos += tft.drawChar(':', xpos, ypos - 8, 7);
    if (mm < 10) xpos += tft.drawChar('0', xpos, ypos, 7);
    xpos += tft.drawNumber(mm, xpos, ypos, 7);
    xsecs = xpos;
    flag = 0;
  }

  if (oss != ss) {
    oss = ss;
    xpos = xsecs;
    if (ss % 2) {
      tft.setTextColor(0x39C4, TFT_BLACK);
      tft.drawChar(':', xcolon, ypos - 8, 7);
      tft.setTextColor(Time_color, TFT_BLACK);
    } else {
      tft.setTextColor(Time_color, TFT_BLACK);
      tft.drawChar(':', xcolon, ypos - 8, 7);
    }
  }

  tft.setTextSize(1);
  prev_hour = timeClient.getHours();
}

// ============================================================
//  UTILITY FUNCTIONS
// ============================================================

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      tftToast("DFPLAYER: TIMEOUT", 2);
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      dfPlayerOK = true; // Card re-inserted
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      dfPlayerOK = false;
      tftToast("SD CARD REMOVED!", 3);
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      dfPlayerOK = true;
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          tftToast("DFPLAYER: CARD NOT FOUND", 3);
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          tftToast("DFPLAYER: FILE NOT FOUND", 3);
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          tftToast("DFPLAYER: FILE MISMATCH", 3);
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void waitMilliseconds(uint16_t msWait) {
  uint32_t start = millis();
  while ((millis() - start) < msWait) {
    delay(1);
  }
}
