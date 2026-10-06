#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI  D7
#define OLED_CLK   D5
#define OLED_DC    D2
#define OLED_CS    D8
#define OLED_RESET D0

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RESET,
  OLED_CS
);

// ---------- Button ----------
#define BUTTON D3       // FLASH button = GPIO0

int networkCount = 0;
int currentNetwork = 0;


// Return a short encryption name
const char* encryptionName(uint8_t encryption) {
  switch (encryption) {
    case ENC_TYPE_NONE:
      return "OPEN";
    case ENC_TYPE_WEP:
      return "WEP";
    case ENC_TYPE_TKIP:
      return "WPA";
    case ENC_TYPE_CCMP:
      return "WPA2";
    case ENC_TYPE_AUTO:
      return "AUTO";
    default:
      return "????";
  }
}


// Display currently selected network
void showNetwork() {

  display.clearDisplay();

  if (networkCount == 0) {
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("No WiFi");

    display.setTextSize(1);
    display.setCursor(0, 25);
    display.println("Try scanning again");

    display.display();
    return;
  }

  String ssid = WiFi.SSID(currentNetwork);

  // ----- Line 1: network number + SSID -----

  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print(currentNetwork + 1);
  display.print("/");
  display.print(networkCount);
  display.print(" ");

  // Maximum roughly 20 characters after number
  if (ssid.length() > 20)
    ssid = ssid.substring(0, 20);

  display.println(ssid);


  // ----- Line 2: encryption -----

  display.setCursor(0, 17);
  display.print("Security: ");
  display.println(
    encryptionName(WiFi.encryptionType(currentNetwork))
  );


  // ----- Line 3: signal strength -----

  display.setCursor(0, 30);
  display.print("Signal:   ");
  display.print(WiFi.RSSI(currentNetwork));
  display.println(" dBm");


  // ----- Line 4: channel -----

  display.setCursor(0, 43);
  display.print("Channel:  ");
  display.println(WiFi.channel(currentNetwork));


  // ----- Bottom hint -----

  display.setCursor(0, 56);
  display.print("FLASH = next");

  display.display();
}


// Scan for WiFi networks
void scanNetworks() {

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Scanning");

  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println("Please wait...");

  display.display();

  networkCount = WiFi.scanNetworks();

  currentNetwork = 0;

  showNetwork();
}


void setup() {

  Serial.begin(115200);

  pinMode(BUTTON, INPUT_PULLUP);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("WiFi");
  display.println("Scanner");

  display.display();

  delay(1000);


  // WiFi setup
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);

  scanNetworks();
}


void loop() {

  // Button pressed
  if (digitalRead(BUTTON) == LOW) {

    unsigned long pressStart = millis();

    // Wait until release
    while (digitalRead(BUTTON) == LOW) {
      delay(10);
    }

    unsigned long pressTime = millis() - pressStart;


    // ----- Long press = rescan -----

    if (pressTime > 1000) {
      scanNetworks();
    }


    // ----- Short press = next network -----

    else {

      if (networkCount > 0) {

        currentNetwork++;

        if (currentNetwork >= networkCount)
          currentNetwork = 0;

        showNetwork();
      }
    }

    delay(100);
  }
}