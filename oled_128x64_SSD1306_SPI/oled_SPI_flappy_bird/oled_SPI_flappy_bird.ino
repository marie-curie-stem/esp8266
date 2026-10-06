// Flappy Bird v1.0 for ESP8266 with 128x64 OLED SPI display SSD1306
// For Marie Curie School in Saigon
// 

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// OLED
// ============================================================

#define SCREEN_WIDTH  128
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

// ============================================================
// BUTTON
// FLASH button = GPIO0 = D3
// ============================================================

#define BUTTON D3

// ============================================================
// GAME PARAMETERS
// ============================================================

const int BIRD_X = 25;
const int BIRD_SIZE = 5;

float birdY;
float birdVelocity;

const float GRAVITY = 0.18;
const float FLAP_VELOCITY = -3.2;

const int PIPE_WIDTH = 13;
const int PIPE_GAP = 22;

float pipeX;
int pipeGapY;

const float PIPE_SPEED = 1.5;

int score;

bool gameOver = false;

unsigned long lastFrame = 0;
const unsigned long FRAME_TIME = 30;   // ~33 FPS


// ============================================================
// CREATE NEW PIPE
// ============================================================

void createPipe() {

  pipeX = SCREEN_WIDTH;

  // Gap can appear between approximately y=14 and y=48
  pipeGapY = random(14, 48);
}


// ============================================================
// RESET GAME
// ============================================================

void startGame() {

  birdY = 30;
  birdVelocity = 0;

  score = 0;

  gameOver = false;

  createPipe();

  display.clearDisplay();
  display.display();
}


// ============================================================
// DRAW BIRD
// ============================================================

void drawBird() {

  int y = (int)birdY;

  // Simple little bird body
  display.fillRect(
    BIRD_X,
    y,
    BIRD_SIZE,
    BIRD_SIZE,
    SSD1306_WHITE
  );

  // Beak
  display.drawPixel(
    BIRD_X + BIRD_SIZE,
    y + 2,
    SSD1306_WHITE
  );

  // Eye
  display.drawPixel(
    BIRD_X + 3,
    y + 1,
    SSD1306_BLACK
  );
}


// ============================================================
// DRAW PIPES
// ============================================================

void drawPipes() {

  int x = (int)pipeX;

  // Top pipe
  display.fillRect(
    x,
    0,
    PIPE_WIDTH,
    pipeGapY - PIPE_GAP / 2,
    SSD1306_WHITE
  );

  // Top pipe lip
  display.fillRect(
    x - 2,
    pipeGapY - PIPE_GAP / 2 - 3,
    PIPE_WIDTH + 4,
    3,
    SSD1306_WHITE
  );


  // Bottom pipe
  int bottomStart = pipeGapY + PIPE_GAP / 2;

  display.fillRect(
    x,
    bottomStart,
    PIPE_WIDTH,
    SCREEN_HEIGHT - bottomStart,
    SSD1306_WHITE
  );

  // Bottom pipe lip
  display.fillRect(
    x - 2,
    bottomStart,
    PIPE_WIDTH + 4,
    3,
    SSD1306_WHITE
  );
}


// ============================================================
// COLLISION DETECTION
// ============================================================

bool checkCollision() {

  int birdTop = (int)birdY;
  int birdBottom = birdTop + BIRD_SIZE;

  int birdLeft = BIRD_X;
  int birdRight = BIRD_X + BIRD_SIZE;


  // Hit ceiling or floor
  if (birdTop <= 0 || birdBottom >= SCREEN_HEIGHT) {
    return true;
  }


  // Check horizontal overlap with pipe
  int pipeLeft = (int)pipeX;
  int pipeRight = pipeLeft + PIPE_WIDTH;

  if (birdRight >= pipeLeft && birdLeft <= pipeRight) {

    int gapTop = pipeGapY - PIPE_GAP / 2;
    int gapBottom = pipeGapY + PIPE_GAP / 2;

    // Bird is outside the gap
    if (birdTop < gapTop || birdBottom > gapBottom) {
      return true;
    }
  }

  return false;
}


// ============================================================
// DRAW SCORE
// ============================================================

void drawScore() {

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(2, 2);
  display.print(score);
}


// ============================================================
// DRAW GAME OVER SCREEN
// ============================================================

void showGameOver() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(20, 10);
  display.println("GAME");

  display.setCursor(27, 30);
  display.println("OVER");

  display.setTextSize(1);
  display.setCursor(40, 50);
  display.print("Score: ");
  display.print(score);

  display.display();
}


// ============================================================
// HANDLE BUTTON
// ============================================================

void handleButton() {

  if (digitalRead(BUTTON) == LOW) {

    // Wait for button release
    while (digitalRead(BUTTON) == LOW) {
      delay(5);
    }

    if (gameOver) {

      // Restart
      startGame();

    } else {

      // Flap
      birdVelocity = FLAP_VELOCITY;
    }
  }
}


// ============================================================
// UPDATE GAME
// ============================================================

void updateGame() {

  // Bird physics
  birdVelocity += GRAVITY;
  birdY += birdVelocity;


  // Move pipe
  pipeX -= PIPE_SPEED;


  // Pipe passed bird
  if (pipeX + PIPE_WIDTH < BIRD_X) {

    score++;

    createPipe();
  }


  // Collision
  if (checkCollision()) {

    gameOver = true;

    showGameOver();
  }
}


// ============================================================
// DRAW GAME
// ============================================================

void drawGame() {

  display.clearDisplay();

  drawPipes();
  drawBird();
  drawScore();

  display.display();
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  randomSeed(analogRead(A0));

  pinMode(BUTTON, INPUT_PULLUP);


  // Start OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {

    // OLED failed
    while (true) {
      delay(1000);
    }
  }

  display.setTextColor(SSD1306_WHITE);


  // Start game
  startGame();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // Button always gets checked
  handleButton();


  // Don't run game physics after game over
  if (gameOver) {
    return;
  }


  // Fixed-ish frame rate
  unsigned long now = millis();

  if (now - lastFrame >= FRAME_TIME) {

    lastFrame = now;

    updateGame();
    drawGame();
  }
}
