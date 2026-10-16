import gc
import time
from machine import SPI, Pin
from ssd1306 import SSD1306_SPI

# ============================================================
# OLED SPI Configuration (ESP8266)  
# ============================================================
SCREEN_WIDTH = 128
SCREEN_HEIGHT = 64

OLED_DC = Pin(4, Pin.OUT)  # D2 (GPIO4)
OLED_RES = Pin(16, Pin.OUT)  # D0 (GPIO16)
OLED_CS = Pin(15, Pin.OUT)  # D8 (GPIO15)

spi = SPI(1, baudrate=10000000, polarity=0, phase=0)
display = SSD1306_SPI(
    SCREEN_WIDTH, SCREEN_HEIGHT, spi, OLED_DC, OLED_RES, OLED_CS
)

# ============================================================
# Input (Button ở chân D3 / GPIO0)
# ============================================================
BUTTON_PIN = 0
button = Pin(BUTTON_PIN, Pin.IN, Pin.PULL_UP)

# ============================================================
# Game constants & State
# ============================================================
GROUND_Y = 56
DINO_X = 10
DINO_W = 14
DINO_H = 16
FRAME_MS = 30
MAX_OBS = 3

GRAVITY = 0.5
JUMP_VEL = -5.6

STATE_START = 0
STATE_PLAY = 1
STATE_OVER = 2

state = STATE_START
dinoY = float(GROUND_Y - DINO_H)
dinoVY = 0.0
onGround = True
score = 0.0
hiScore = 0
speed = 3.0
nextSpawnDist = 60.0
groundScroll = 0.0
animTick = 0
lastButton = False
overTime = 0

# Hàm sinh số ngẫu nhiên độc lập, tương thích 100% mọi bản MicroPython ESP8266
def my_rand(a, b):
  return a + (time.ticks_us() % (b - a + 1))


# Tối ưu RAM: Mảng list cố định [x, y, w, h, is_bird, active]
obs = [[0.0, 0, 0, 0, False, False] for _ in range(MAX_OBS)]


def button_pressed_edge():
  global lastButton
  now = button.value() == 0
  edge = now and not lastButton
  lastButton = now
  return edge


def reset_game():
  global dinoY, dinoVY, onGround, score, speed, groundScroll, nextSpawnDist, animTick
  gc.collect()
  for o in obs:
    o[5] = False  # active = False
  dinoY = float(GROUND_Y - DINO_H)
  dinoVY = 0.0
  onGround = True
  score = 0.0
  speed = 3.0
  groundScroll = 0.0
  nextSpawnDist = 60.0
  animTick = 0


def spawn_obstacle():
  global nextSpawnDist
  for o in obs:
    if o[5]:
      continue
    o[5] = True  # active = True
    o[0] = float(SCREEN_WIDTH + 2)  # x
    makeBird = (score > 150) and (my_rand(0, 99) < 30)
    if makeBird:
      o[4] = True  # bird = True
      o[2] = 14  # w
      o[3] = 8  # h
      o[1] = GROUND_Y - 10 if my_rand(0, 1) == 0 else GROUND_Y - DINO_H - 12
    else:
      o[4] = False  # bird = False
      o[2] = 5 if my_rand(0, 1) == 0 else 9
      o[3] = my_rand(10, 18)
      o[1] = GROUND_Y - o[3]
    nextSpawnDist = float(my_rand(70, 130)) + speed * 12 + o[2]
    return


def draw_dino(x, y, dead):
  iy = int(y)
  display.fill_rect(x + 7, iy, 7, 6, 1)
  display.pixel(x + 11, iy + 1, 0)
  if dead:
    display.pixel(x + 12, iy + 2, 0)
  display.hline(x + 10, iy + 4, 4, 0)
  display.fill_rect(x + 8, iy + 5, 3, 2, 1)
  display.fill_rect(x + 2, iy + 6, 9, 6, 1)
  display.fill_rect(x, iy + 6, 3, 3, 1)
  display.fill_rect(x + 11, iy + 8, 2, 1, 1)
  if not onGround or dead:
    display.fill_rect(x + 3, iy + 12, 2, 4, 1)
    display.fill_rect(x + 8, iy + 12, 2, 4, 1)
  elif (animTick // 4) % 2 == 0:
    display.fill_rect(x + 3, iy + 12, 2, 4, 1)
    display.fill_rect(x + 8, iy + 12, 2, 2, 1)
  else:
    display.fill_rect(x + 3, iy + 12, 2, 2, 1)
    display.fill_rect(x + 8, iy + 12, 2, 4, 1)


def draw_cactus(x, y, w, h):
  cx = x + w // 2
  display.fill_rect(cx - 1, y, 3, h, 1)
  if w >= 7:
    display.fill_rect(x, y + 4, 2, 5, 1)
    display.hline(x, y + 8, cx - x, 1)
    display.fill_rect(x + w - 2, y + 2, 2, 5, 1)
    display.hline(cx + 1, y + 6, x + w - cx - 1, 1)
  else:
    display.fill_rect(x, y + 3, 1, 3, 1)
    display.fill_rect(x + w - 1, y + 5, 1, 3, 1)


def draw_bird(x, y):
  display.fill_rect(x + 3, y + 3, 9, 3, 1)
  display.fill_rect(x, y + 3, 3, 1, 1)
  display.fill_rect(x + 11, y + 2, 3, 2, 1)
  if (animTick // 5) % 2 == 0:
    display.line(x + 5, y + 3, x + 8, y, 1)
    display.line(x + 6, y + 3, x + 9, y, 1)
  else:
    display.line(x + 5, y + 5, x + 8, y + 7, 1)
    display.line(x + 6, y + 5, x + 9, y + 7, 1)


def draw_ground():
  display.hline(0, GROUND_Y, SCREEN_WIDTH, 1)
  for i in range(12):
    x = int(i * 23 - groundScroll) % SCREEN_WIDTH
    if x < 0:
      x += SCREEN_WIDTH
    display.pixel(x, GROUND_Y + 3 + (i % 3) * 2, 1)
    display.pixel((x + 1) % SCREEN_WIDTH, GROUND_Y + 3 + (i % 3) * 2, 1)


def draw_score():
  buf = "HI %05d %05d" % (hiScore, int(score))
  display.text(buf, SCREEN_WIDTH - len(buf) * 8, 0, 1)


def collides():
  dx1 = float(DINO_X + 2)
  dx2 = float(DINO_X + DINO_W - 2)
  dy1 = dinoY + 2
  dy2 = dinoY + DINO_H - 1
  for o in obs:
    if not o[5]:
      continue
    ox1 = o[0] + 1
    ox2 = o[0] + o[2] - 1
    oy1 = float(o[1] + 1)
    oy2 = float(o[1] + o[3] - 1)
    if dx1 < ox2 and dx2 > ox1 and dy1 < oy2 and dy2 > oy1:
      return True
  return False


def update_game(jumpPressed):
  global dinoVY, dinoY, onGround, score, speed, groundScroll, nextSpawnDist, animTick, state, hiScore, overTime
  if jumpPressed and onGround:
    dinoVY = JUMP_VEL
    onGround = False

  if not onGround:
    dinoVY += GRAVITY
    dinoY += dinoVY
    if dinoY >= float(GROUND_Y - DINO_H):
      dinoY = float(GROUND_Y - DINO_H)
      dinoVY = 0.0
      onGround = True

  score += 0.2
  speed = 3.0 + score * 0.004
  if speed > 6.5:
    speed = 6.5

  groundScroll += speed
  if groundScroll > 100000:
    groundScroll = 0.0
  animTick += 1

  for o in obs:
    if not o[5]:
      continue
    o[0] -= speed
    if o[0] + o[2] < 0:
      o[5] = False

  nextSpawnDist -= speed
  if nextSpawnDist <= 0:
    spawn_obstacle()

  if collides():
    state = STATE_OVER
    overTime = time.ticks_ms()
    if int(score) > hiScore:
      hiScore = int(score)


def draw_game(dead):
  display.fill(0)
  draw_ground()
  for o in obs:
    if not o[5]:
      continue
    if o[4]:
      draw_bird(int(o[0]), o[1])
    else:
      draw_cactus(int(o[0]), o[1], o[2], o[3])
  draw_dino(DINO_X, dinoY, dead)
  draw_score()


# ============================================================
# Main Loop
# ============================================================
reset_game()
lastFrame = time.ticks_ms()

while True:
  now_ms = time.ticks_ms()
  if time.ticks_diff(now_ms, lastFrame) < FRAME_MS:
    time.sleep_ms(2)
    continue
  lastFrame = now_ms

  pressed = button_pressed_edge()

  if state == STATE_START:
    draw_game(False)
    display.text("Marie Curie", 32, 20, 1)
    display.text("Press to start", 10, 32, 1)
    display.show()
    if pressed:
      reset_game()
      state = STATE_PLAY

  elif state == STATE_PLAY:
    update_game(pressed)
    draw_game(state == STATE_OVER)
    display.show()
    if animTick % 30 == 0:
      gc.collect()

  elif state == STATE_OVER:
    draw_game(True)
    display.text("GAME OVER", 28, 18, 1)
    display.text("Press to restart", 8, 30, 1)
    display.show()
    if pressed and time.ticks_diff(time.ticks_ms(), overTime) > 500:
      reset_game()
      state = STATE_PLAY
