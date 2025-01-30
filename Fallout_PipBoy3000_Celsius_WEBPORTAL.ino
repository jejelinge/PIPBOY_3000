// TFT_eSPI_memory
//
// Example sketch which shows how to display an
// animated GIF image stored in FLASH memory

// Adapted by Bodmer for the TFT_eSPI Arduino library:
// https://github.com/Bodmer/TFT_eSPI
//
// To display a GIF from memory, a single callback function
// must be provided - GIFDRAW
// This function is called after each scan line is decoded
// and is passed the 8-bit pixels, RGB565 palette and info
// about how and where to display the line. The palette entries
// can be in little-endian or big-endian order; this is specified
// in the begin() method.
//
// The AnimatedGIF class doesn't allocate or free any memory, but the
// instance data occupies about 22.5K of RAM.

//========================USEFUL VARIABLES=============================
int UTC = 2; //Set your time zone ex: france = UTC+2
uint16_t notification_volume = 30;  
//=====================================================================
bool res;
// Load GIF library
#include <AnimatedGIF.h>
AnimatedGIF gif;

#include ".\images/INIT.h"
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

#define INIT INIT
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
#define Light_green 0x35C2
#define Dark_green 0x0261 
#define Time_color 0x04C0

#include "WiFiManager.h"
#include "NTPClient.h"
#include "DFRobotDFPlayerMini.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_SHT31.h"

const byte RXD2 = 16;  // Connects to module's TX => 16
const byte TXD2 = 17;  // Connects to module's RX => 17

DFRobotDFPlayerMini myDFPlayer;
void printDetail(uint8_t type, int value);
#define FPSerial Serial1
TFT_eSPI tft = TFT_eSPI();
Adafruit_SHT31 sht31 = Adafruit_SHT31();

int i=0;
int a=0;
uint16_t x = 0, y = 0;
int interupt = 1;
float t_far = 0;
int hh=0;
int mm=0;
int ss=0;
int flag = 0;
int prev_hour = 0;
String localip ;
bool enableHeater = false;
uint8_t loopCnt = 0;
const long utcOffsetInSeconds = 3600; // Offset in second
uint32_t targetTime = 0;                    // for next 1 second timeout
static uint8_t conv2d(const char* p); // Forward declaration needed for IDE 1.6.x

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds*UTC);

byte omm = 99, oss = 99;
byte xcolon = 0, xsecs = 0;
unsigned int colour = 0;

void setup() {

  pinMode(IN_RADIO, INPUT_PULLUP);
  pinMode(IN_STAT, INPUT_PULLUP);
  pinMode(IN_DATA, INPUT_PULLUP);
  pinMode(IN_INV, INPUT_PULLUP);
  pinMode(IN_TIME, INPUT_PULLUP);

  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(Light_green, TFT_BLACK);
  tft.drawString("Enter SSID and password", 10, 20, 4);
 
    WiFiManager manager;    
    //manager.resetSettings();
   manager.setTimeout(180);
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "BTTF_CLOCK" and waits in a blocking loop for configuration
  res = manager.autoConnect("PIPBOY3000","password");
  
  if(!res) {
  Serial.println("failed to connect and timeout occurred");
  ESP.restart(); //reset and try again
  }

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    tft.drawString(".", 10 + a, 40, 4);
    a=a+5;
    if (a>100){
     tft.fillScreen(TFT_BLACK);
     tft.drawString("ERROR",180 , 20, 4);
     tft.drawString("Check wifi SSID and PASSWORD", 10, 60, 4);
    }
  }
  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  tft.fillScreen(TFT_BLACK);
  localip = WiFi.localIP().toString();
  tft.drawString(localip, 10, 20, 4);

  timeClient.begin();

  FPSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  tft.drawString("Initializing DFPlayer...", 10, 20, 4);
  tft.fillScreen(TFT_BLACK);

  if (!myDFPlayer.begin(FPSerial, /*isACK = */ true, /*doReset = */ true)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin DFplayer"));
    tft.drawString("Unable to begin DFplayer", 10, 20, 4);
    Serial.println(F("1.Recheck the connection!"));
    tft.drawString("1. Recheck the connection", 10, 50, 4);
    Serial.println(F("2.Insert the SD card!"));
    tft.drawString("2. Insert the SD card", 10, 80, 4);
    tft.drawString("3. Format SD card in FAT32", 10, 110, 4);

    while (true) {
      delay(0);
    }
  }

Serial.println(F("DFPlayer Mini online."));
tft.drawString("DFPlayer Mini online", 10, 20, 4);
myDFPlayer.volume(notification_volume);
myDFPlayer.setTimeOut(500);

  while (!Serial)
    delay(10);    // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
     tft.fillScreen(TFT_BLACK);
    Serial.println("Couldn't find SHT31");
    tft.drawString("Couldn't find SHT31 (temp sensor)", 10, 20, 4);
    tft.drawString("Recheck the connection", 10, 50, 4);
    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");

  gif.begin(BIG_ENDIAN_PIXELS);
  
  delay(1000);
  myDFPlayer.playMp3Folder(1);  //Play the first mp3

  if (gif.open((uint8_t *)INIT, sizeof(INIT), GIFDraw))
  {
    tft.startWrite(); // The TFT chip select is locked low
    while (gif.playFrame(true, NULL))
    {
      yield();
    }

    gif.close();
    tft.endWrite(); // Release TFT chip select for other SPI devices
  }

}

void loop()
{

  timeClient.update();
  Serial.print("Time: ");
  Serial.println(timeClient.getFormattedTime());
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);
  
  int monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  int currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  if (digitalRead(IN_STAT) == false) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    while (digitalRead(IN_STAT) == false) {
      if (gif.open((uint8_t *)STAT, sizeof(STAT), GIFDraw)) {
        //Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
    }
  }

  if (digitalRead(IN_INV) == false) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    while (digitalRead(IN_INV) == false) {
      if (gif.open((uint8_t *)INV, sizeof(INV), GIFDraw)) {
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
    }
  }

  if (digitalRead(IN_DATA) == false) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);
    tft.drawBitmap(35, 80, temperatureTemp_humTemp_hum_2, 408, 29, Light_green);
    //tft.drawBitmap(35, 80, temperatureTemp_hum_F , 408, 29, Light_green);
    tft.drawBitmap(200, 200, RadiationRadiation, 62, 61, Light_green);

    while (digitalRead(IN_DATA) == false) {

      float t = sht31.readTemperature();
      float h = sht31.readHumidity();
      if (gif.open((uint8_t *)DATA_1, sizeof(DATA_1), GIFDraw)) {
        // Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
      //show_hour();
      tft.setTextColor(Time_color, TFT_BLACK);
      t_far = (t*1.8)+32;
      tft.drawFloat(t, 2, 60, 135, 7);
      tft.drawFloat(h, 2, 258, 135, 7);
    }
  }

  if (digitalRead(IN_TIME) == false) {
    myDFPlayer.playMp3Folder(random(2, 5));
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2, 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);
    while (digitalRead(IN_TIME) == false) {
      if (gif.open((uint8_t *)TIME, sizeof(TIME), GIFDraw)) {
        // Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
        tft.startWrite();  // The TFT chip select is locked low
        while (gif.playFrame(true, NULL)) {
          yield();
        }
        gif.close();
        tft.endWrite();  // Release TFT chip select for other SPI devices
      }
      show_hour();
    }
  }

  if(digitalRead(IN_RADIO) == false ) {
    flag = 1;
    myDFPlayer.playMp3Folder(random(2, 5));
    delay(500);
    myDFPlayer.playMp3Folder(random(5, 10));
    tft.fillScreen(TFT_BLACK);
    tft.drawBitmap(35, 300, Bottom_layer_2Bottom_layer_2 , 380, 22, Dark_green);
    tft.drawBitmap(35, 300, myBitmapDate, 380, 22, Light_green);
  
  while(digitalRead(IN_RADIO) == false) {
    if (gif.open((uint8_t *)RADIO, sizeof(RADIO), GIFDraw))
    {
      //Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
      tft.startWrite(); // The TFT chip select is locked low
      while (gif.playFrame(true, NULL))
      {
        yield();
      }
      gif.close();
      tft.endWrite(); // Release TFT chip select for other SPI devices
    }
  }
  }
     
}

void show_hour(){
  tft.setTextSize(2);
  mm = timeClient.getMinutes();
  ss = timeClient.getSeconds();

  if (timeClient.getHours() == 0) {
    hh = 12;
  }

  else if (timeClient.getHours() == 12) {
    hh = timeClient.getHours();
  }

  else if (timeClient.getHours() >= 13) {
    hh = timeClient.getHours() - 12;
  }

  else {
    hh = timeClient.getHours();
  }

   //tft.fillRect(140, 210, 200, 50, TFT_BLACK);
   if(timeClient.getHours() != prev_hour){tft.fillRect(140, 210, 200, 50, TFT_BLACK);}

   if(timeClient.getHours() <12 && timeClient.getHours() >0) {tft.drawBitmap(150, 220, MorningMorning , 170, 29, Light_green);}
   else {tft.drawBitmap(150, 220, afternoonAfternoon  , 170, 29, Light_green);}
  

    // Update digital time
    int xpos = 85;
    int ypos = 90; // Top left corner ot clock text, about half way down
    int ysecs = ypos + 24;

    if (omm != mm || flag == 1) { // Redraw hours and minutes time every minute
      omm = mm;
      // Draw hours and minutes
      tft.setTextColor(Time_color, TFT_BLACK); 
      if (hh < 10) xpos += tft.drawChar('0', xpos, ypos, 7); // Add hours leading zero for 24 hr clock
      xpos += tft.drawNumber(hh, xpos, ypos, 7);             // Draw hours
      xcolon = xpos; // Save colon coord for later to flash on/off later
      xpos += tft.drawChar(':', xpos, ypos - 8, 7);
      if (mm < 10) xpos += tft.drawChar('0', xpos, ypos, 7); // Add minutes leading zero
      xpos += tft.drawNumber(mm, xpos, ypos, 7);             // Draw minutes
      xsecs = xpos; // Sae seconds 'x' position for later display updates
      flag = 0;
    }
    if (oss != ss) { // Redraw seconds time every second
      oss = ss;
      xpos = xsecs;

      if (ss % 2) { // Flash the colons on/off
        tft.setTextColor(0x39C4, TFT_BLACK);        // Set colour to grey to dim colon
        tft.drawChar(':', xcolon, ypos - 8, 7);     // Hour:minute colon
        tft.setTextColor(Time_color, TFT_BLACK);    // Set colour back to yellow
      }
      else {
        tft.setTextColor(Time_color, TFT_BLACK);
        tft.drawChar(':', xcolon, ypos - 8, 7);     // Hour:minute colon
      }

    }
  // }
 tft.setTextSize(1); 
 prev_hour = timeClient.getHours();
}

// Function to extract numbers from compile time string
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
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
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
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
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
    // calling mp3.loop() periodically allows for notifications
    // to be handled without interrupts
    delay(1);
  }
}
