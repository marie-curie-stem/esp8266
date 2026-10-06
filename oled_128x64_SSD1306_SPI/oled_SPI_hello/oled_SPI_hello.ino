// 

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_MOSI D7
#define OLED_CLK  D5
#define OLED_DC   D2
#define OLED_CS   D8
#define OLED_RESET D0

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &SPI,
  OLED_DC,
  OLED_RESET,
  OLED_CS
);

void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Hello!");

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("ESP8266 + OLED");

  display.setCursor(0, 34);
  display.println("Marie Curie School");

  display.setCursor(0, 42);
  display.println("Saigon");

  display.setCursor(0, 56);
  display.println("Programming Club 2026");

  display.display();
}

void loop() {
}