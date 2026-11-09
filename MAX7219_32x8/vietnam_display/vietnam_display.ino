// ESP8266 D1 mini + 4x MAX7219 (FC-16, 32x8 pixels)
// Scrolling Vietnamese text + small animations.
//
// Wiring: VCC->5V, GND->G, DIN->D7, CLK->D5, CS->D8
// Libraries: MD_Parola + MD_MAX72XX (Library Manager)

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   4
#define CS_PIN        D8

#define WIDTH         (MAX_DEVICES * 8)
#define HEIGHT        8

#define SCROLL_SPEED  30   // ms per scroll step: LOWER = FASTER (was 50)
#define BRIGHTNESS    3    // 0..15

// If the animations (not the text) appear mirrored or upside down, toggle these:
const bool MIRROR_X = false;
const bool FLIP_Y   = false;

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MD_MAX72XX *mx;

// ---------- Text (Vietnamese without diacritics, see note in chat) ----------
const char* const phrases[] = {
  "Xin chao! Chuc ban mot ngay that vui ve!",
  "Chuc mung nam moi - Van su nhu y!",
  "Ca phe sua da - ngon tuyet!",
  "Pho bo, banh mi, bun cha - doi bung roi!",
  "Mot, hai, ba - Yo!",
  "Chuc ngu ngon!"
};
const uint8_t PHRASES = sizeof(phrases) / sizeof(phrases[0]);

// ---------- Sprites (8x8, bit 7 = leftmost pixel) ----------
const uint8_t HEART_BIG[8]   = {0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00};
const uint8_t HEART_SMALL[8] = {0x00, 0x24, 0x7E, 0x7E, 0x3C, 0x18, 0x00, 0x00};
const uint8_t SMILEY[8]      = {0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C};
const uint8_t SMILEY_WINK[8] = {0x3C, 0x42, 0x85, 0x81, 0xA5, 0x99, 0x42, 0x3C};
const uint8_t PAC_OPEN[8]    = {0x3C, 0x7E, 0xF8, 0xE0, 0xE0, 0xF8, 0x7E, 0x3C};
const uint8_t PAC_CLOSED[8]  = {0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C};
const uint8_t GHOST[8]       = {0x3C, 0x7E, 0xDB, 0xDB, 0xFF, 0xFF, 0xFF, 0xA5};

// ---------- Drawing helpers ----------
// x: 0 = left ... 31 = right, y: 0 = top ... 7 = bottom
void px(int x, int y, bool on = true) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  int col = MIRROR_X ? x : (WIDTH - 1 - x);
  int row = FLIP_Y ? (HEIGHT - 1 - y) : y;
  mx->setPoint(row, col, on);
}

void drawSprite(const uint8_t *s, int x) {
  for (int r = 0; r < 8; r++)
    for (int b = 0; b < 8; b++)
      if (s[r] & (0x80 >> b)) px(x + b, r);
}

// Batch all pixel changes of one frame, then push them to the display at once
void beginFrame() {
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx->clear();
}

void endFrame() {
  mx->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

// ---------- Text scrolling ----------
void scrollText(const char *msg) {
  display.displayClear();
  display.displayText(msg, PA_LEFT, SCROLL_SPEED, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!display.displayAnimate()) yield();
}

// ---------- Animations ----------
void heartBeat() {
  for (int i = 0; i < 10; i++) {
    bool big = (i % 2);
    beginFrame();
    drawSprite(big ? HEART_BIG : HEART_SMALL, 2);
    drawSprite(big ? HEART_BIG : HEART_SMALL, 12);
    drawSprite(big ? HEART_BIG : HEART_SMALL, 22);
    endFrame();
    delay(big ? 250 : 350);
  }
}

void smileyWink() {
  for (int x = -8; x <= 12; x++) {            // slide in
    beginFrame();
    drawSprite(SMILEY, x);
    endFrame();
    delay(50);
  }
  for (int i = 0; i < 4; i++) {               // wink a few times
    beginFrame(); drawSprite(SMILEY_WINK, 12); endFrame(); delay(250);
    beginFrame(); drawSprite(SMILEY, 12);      endFrame(); delay(500);
  }
  for (int x = 12; x < WIDTH + 1; x++) {      // slide out
    beginFrame();
    drawSprite(SMILEY, x);
    endFrame();
    delay(40);
  }
}

void pacmanChase() {
  for (int p = -24; p < WIDTH + 8; p++) {
    beginFrame();
    drawSprite((((p + 24) / 2) & 1) ? PAC_OPEN : PAC_CLOSED, p);
    drawSprite(GHOST, p + 12);
    endFrame();
    delay(70);
  }
}

void bouncingBall() {
  int x = 0, y = 0, dx = 1, dy = 1;
  for (int f = 0; f < 250; f++) {
    beginFrame();
    px(x, y); px(x + 1, y); px(x, y + 1); px(x + 1, y + 1);   // 2x2 ball
    endFrame();
    x += dx; y += dy;
    if (x <= 0 || x >= WIDTH - 2)  dx = -dx;
    if (y <= 0 || y >= HEIGHT - 2) dy = -dy;
    delay(40);
  }
}

void sineWave() {
  for (int t = 0; t < 240; t++) {
    beginFrame();
    for (int x = 0; x < WIDTH; x++) {
      int y = (int)round(3.5 + 3.5 * sin((x + t) * 0.35));
      px(x, y);
      px(x, HEIGHT - 1 - y);                  // mirrored wave -> double helix
    }
    endFrame();
    delay(40);
  }
}

void matrixRain() {
  int drop[WIDTH];
  for (int x = 0; x < WIDTH; x++) drop[x] = -random(0, 16);
  for (int f = 0; f < 160; f++) {
    beginFrame();
    for (int x = 0; x < WIDTH; x++) {
      for (int k = 0; k < 3; k++) px(x, drop[x] - k);          // 3-pixel tail
      drop[x]++;
      if (drop[x] - 2 >= HEIGHT) drop[x] = -random(0, 20);
    }
    endFrame();
    delay(80);
  }
}

void equalizer() {
  const int BARS = WIDTH / 2;
  int h[BARS], target[BARS];
  for (int b = 0; b < BARS; b++) { h[b] = 0; target[b] = 0; }
  for (int f = 0; f < 160; f++) {
    beginFrame();
    for (int b = 0; b < BARS; b++) {
      if (random(4) == 0) target[b] = random(1, HEIGHT + 1);
      if (h[b] < target[b]) h[b]++;
      else if (h[b] > target[b]) h[b]--;
      for (int k = 0; k < h[b]; k++) px(2 * b, HEIGHT - 1 - k);
    }
    endFrame();
    delay(60);
  }
}

void (*const animations[])() = {
  heartBeat, smileyWink, pacmanChase, bouncingBall, sineWave, matrixRain, equalizer
};
const uint8_t ANIMS = sizeof(animations) / sizeof(animations[0]);

// ---------- Main ----------
void setup() {
  display.begin();
  mx = display.getGraphicObject();
  display.setIntensity(BRIGHTNESS);
  display.displayClear();
  randomSeed(analogRead(A0));
}

void loop() {
  static uint8_t n = 0;
  scrollText(phrases[n % PHRASES]);
  animations[n % ANIMS]();
  n++;
}
