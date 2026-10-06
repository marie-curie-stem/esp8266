// ============================================================
// Jumping Dino for ESP8266 + SSD1306 128x64 OLED (SPI)
// Button: D3 (GPIO0 = FLASH button on NodeMCU/Wemos) to GND
// Libraries: Adafruit GFX, Adafruit SSD1306
// ============================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// OLED
// ============================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI D7
#define OLED_CLK  D5
#define OLED_DC   D2
#define OLED_CS   D8
#define OLED_RESET D0

// Hardware SPI (MOSI = D7, CLK = D5 are fixed by the ESP8266)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI,
                         OLED_DC, OLED_RESET, OLED_CS);

// ============================================================
// Input
// ============================================================
#define BUTTON_PIN D3   // active LOW, internal pull-up

// ============================================================
// Game constants
// ============================================================
#define GROUND_Y     56
#define DINO_X       10
#define DINO_W       14
#define DINO_H       16
#define FRAME_MS     30
#define MAX_OBS      3

const float GRAVITY    = 0.5f;
const float JUMP_VEL   = -5.6f;

struct Obstacle {
  float   x;
  int16_t y;      // top
  uint8_t w, h;
  bool    bird;
  bool    active;
};

enum State { STATE_START, STATE_PLAY, STATE_OVER };

State    state = STATE_START;
Obstacle obs[MAX_OBS];

float    dinoY;           // top of dino
float    dinoVY;
bool     onGround;
float    score;
int      hiScore = 0;
float    speed;
float    nextSpawnDist;   // pixels until next obstacle spawns
float    groundScroll;
uint8_t  animTick;
unsigned long lastFrame = 0;
unsigned long overTime  = 0;
bool     lastButton = false;

// ============================================================
// Helpers
// ============================================================
bool buttonPressedEdge() {
  bool now = (digitalRead(BUTTON_PIN) == LOW);
  bool edge = now && !lastButton;
  lastButton = now;
  return edge;
}

void resetGame() {
  for (int i = 0; i < MAX_OBS; i++) obs[i].active = false;
  dinoY = GROUND_Y - DINO_H;
  dinoVY = 0;
  onGround = true;
  score = 0;
  speed = 3.0f;
  groundScroll = 0;
  nextSpawnDist = 60;
  animTick = 0;
}

void spawnObstacle() {
  for (int i = 0; i < MAX_OBS; i++) {
    if (obs[i].active) continue;
    Obstacle &o = obs[i];
    o.active = true;
    o.x = SCREEN_WIDTH + 2;
    bool makeBird = (score > 150) && (random(100) < 30);
    if (makeBird) {
      o.bird = true;
      o.w = 14;
      o.h = 8;
      // 50%: low bird (jump over it), 50%: high bird (stay on the ground)
      if (random(2) == 0) o.y = GROUND_Y - 10;
      else                o.y = GROUND_Y - DINO_H - 12;
    } else {
      o.bird = false;
      o.w = random(2) == 0 ? 5 : 9;
      o.h = random(10, 19);
      o.y = GROUND_Y - o.h;
    }
    // gap until next obstacle: scales with speed so it stays jumpable
    nextSpawnDist = random(70, 130) + speed * 12 + o.w;
    return;
  }
}

void drawDino(int x, int y, bool dead) {
  // head
  display.fillRect(x + 7, y, 7, 6, WHITE);
  // eye
  display.drawPixel(x + 11, y + 1, BLACK);
  if (dead) display.drawPixel(x + 12, y + 2, BLACK);
  // mouth
  display.drawFastHLine(x + 10, y + 4, 4, BLACK);
  // neck + body
  display.fillRect(x + 8, y + 5, 3, 2, WHITE);
  display.fillRect(x + 2, y + 6, 9, 6, WHITE);
  // tail
  display.fillRect(x, y + 6, 3, 3, WHITE);
  // arm
  display.fillRect(x + 11, y + 8, 2, 1, WHITE);
  // legs
  if (!onGround || dead) {
    display.fillRect(x + 3, y + 12, 2, 4, WHITE);
    display.fillRect(x + 8, y + 12, 2, 4, WHITE);
  } else if ((animTick / 4) % 2 == 0) {
    display.fillRect(x + 3, y + 12, 2, 4, WHITE);
    display.fillRect(x + 8, y + 12, 2, 2, WHITE);
  } else {
    display.fillRect(x + 3, y + 12, 2, 2, WHITE);
    display.fillRect(x + 8, y + 12, 2, 4, WHITE);
  }
}

void drawCactus(int x, int y, int w, int h) {
  int cx = x + w / 2;
  display.fillRect(cx - 1, y, 3, h, WHITE);                  // stem
  if (w >= 7) {
    display.fillRect(x, y + 4, 2, 5, WHITE);                 // left arm
    display.fillRect(x, y + 8, cx - x, 1, WHITE);
    display.fillRect(x + w - 2, y + 2, 2, 5, WHITE);         // right arm
    display.fillRect(cx + 1, y + 6, x + w - cx - 1, 1, WHITE);
  } else {
    display.fillRect(x, y + 3, 1, 3, WHITE);
    display.fillRect(x + w - 1, y + 5, 1, 3, WHITE);
  }
}

void drawBird(int x, int y) {
  display.fillRect(x + 3, y + 3, 9, 3, WHITE);   // body
  display.fillRect(x, y + 3, 3, 1, WHITE);       // tail
  display.fillRect(x + 11, y + 2, 3, 2, WHITE);  // head/beak
  if ((animTick / 5) % 2 == 0) {                 // wing up
    display.drawLine(x + 5, y + 3, x + 8, y, WHITE);
    display.drawLine(x + 6, y + 3, x + 9, y, WHITE);
  } else {                                       // wing down
    display.drawLine(x + 5, y + 5, x + 8, y + 7, WHITE);
    display.drawLine(x + 6, y + 5, x + 9, y + 7, WHITE);
  }
}

void drawGround() {
  display.drawFastHLine(0, GROUND_Y, SCREEN_WIDTH, WHITE);
  for (int i = 0; i < 12; i++) {
    int x = (int)(i * 23 - groundScroll) % SCREEN_WIDTH;
    if (x < 0) x += SCREEN_WIDTH;
    display.drawPixel(x, GROUND_Y + 3 + (i % 3) * 2, WHITE);
    display.drawPixel((x + 1) % SCREEN_WIDTH, GROUND_Y + 3 + (i % 3) * 2, WHITE);
  }
}

void drawScore() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  char buf[24];
  snprintf(buf, sizeof(buf), "HI %05d %05d", hiScore, (int)score);
  display.setCursor(SCREEN_WIDTH - 13 * 6, 0);
  display.print(buf);
}

bool collides() {
  // shrink hitboxes slightly to be forgiving
  float dx1 = DINO_X + 2, dx2 = DINO_X + DINO_W - 2;
  float dy1 = dinoY + 2,  dy2 = dinoY + DINO_H - 1;
  for (int i = 0; i < MAX_OBS; i++) {
    if (!obs[i].active) continue;
    float ox1 = obs[i].x + 1, ox2 = obs[i].x + obs[i].w - 1;
    float oy1 = obs[i].y + 1, oy2 = obs[i].y + obs[i].h - 1;
    if (dx1 < ox2 && dx2 > ox1 && dy1 < oy2 && dy2 > oy1) return true;
  }
  return false;
}

// ============================================================
// Game logic
// ============================================================
void updateGame(bool jumpPressed) {
  // jump
  if (jumpPressed && onGround) {
    dinoVY = JUMP_VEL;
    onGround = false;
  }
  // also allow holding the button for a jump right after landing
  if (!onGround) {
    dinoVY += GRAVITY;
    dinoY += dinoVY;
    if (dinoY >= GROUND_Y - DINO_H) {
      dinoY = GROUND_Y - DINO_H;
      dinoVY = 0;
      onGround = true;
    }
  }

  // speed + score
  score += 0.2f;
  speed = 3.0f + score * 0.004f;
  if (speed > 6.5f) speed = 6.5f;

  groundScroll += speed;
  if (groundScroll > 100000) groundScroll = 0;
  animTick++;

  // move obstacles
  for (int i = 0; i < MAX_OBS; i++) {
    if (!obs[i].active) continue;
    obs[i].x -= speed;
    if (obs[i].x + obs[i].w < 0) obs[i].active = false;
  }

  // spawn
  nextSpawnDist -= speed;
  if (nextSpawnDist <= 0) spawnObstacle();

  // collision
  if (collides()) {
    state = STATE_OVER;
    overTime = millis();
    if ((int)score > hiScore) hiScore = (int)score;
  }
}

void drawGame(bool dead) {
  display.clearDisplay();
  drawGround();
  for (int i = 0; i < MAX_OBS; i++) {
    if (!obs[i].active) continue;
    if (obs[i].bird) drawBird((int)obs[i].x, obs[i].y);
    else             drawCactus((int)obs[i].x, obs[i].y, obs[i].w, obs[i].h);
  }
  drawDino(DINO_X, (int)dinoY, dead);
  drawScore();
}

// ============================================================
// Arduino
// ============================================================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) delay(1000);
  }
  randomSeed(analogRead(A0) ^ micros());
  display.clearDisplay();
  display.display();
  resetGame();
}

void loop() {
  if (millis() - lastFrame < FRAME_MS) return;
  lastFrame = millis();

  bool pressed = buttonPressedEdge();

  switch (state) {
    case STATE_START:
      drawGame(false);
      display.setTextSize(1);
      display.setCursor(22, 20);
      display.print(F("DINO RUN"));
      display.setCursor(10, 32);
      display.print(F("Press button to start"));
      display.display();
      if (pressed) {
        resetGame();
        state = STATE_PLAY;
      }
      break;

    case STATE_PLAY:
      updateGame(pressed);
      drawGame(state == STATE_OVER);
      display.display();
      break;

    case STATE_OVER:
      drawGame(true);
      display.setTextSize(1);
      display.setCursor(34, 18);
      display.print(F("GAME OVER"));
      display.setCursor(16, 30);
      display.print(F("Press to restart"));
      display.display();
      // short lockout so a panicked button press doesn't skip the screen
      if (pressed && millis() - overTime > 500) {
        resetGame();
        state = STATE_PLAY;
      }
      break;
  }
}
