// ============================================================================
//  PONG GAME — Arduino UNO
//  8x MAX7219 (2 rows × 4 cols = 32×16), 2× HC-SR04, Button, Buzzer, LCD2004
//  Architecture : State-machine  ·  Non-blocking  ·  millis() only
// ============================================================================

// ────────────────────────── LIBRARIES ───────────────────────────────────────
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

// ============================================================================
// ██  CONFIGURABLE PARAMETERS — edit these to tune the game
// ============================================================================

// — MAX7219 daisy-chain --------------------------------------------------
#define MAX_CS_PIN          10      // Chip-select (directly on SPI bus)
#define NUM_MODULES         8       // Total MAX7219 modules
#define COLS_PER_ROW        4       // Modules per horizontal row
#define ROWS_OF_MODULES     2       // Rows of modules

// — Display resolution (derived) ----------------------------------------
#define DISP_W              (COLS_PER_ROW * 8)    // 32 pixels wide
#define DISP_H              (ROWS_OF_MODULES * 8) // 16 pixels tall

// — HC-SR04 ultrasonic sensors ------------------------------------------
#define US1_TRIG_PIN        2       // Player 1 (left paddle)
#define US1_ECHO_PIN        3
#define US2_TRIG_PIN        4       // Player 2 (right paddle)
#define US2_ECHO_PIN        5
#define PADDLE_DIST_CM      30      // Threshold distance to activate paddle (cm)
#define US_TIMEOUT_US       20000   // Echo timeout in µs (~340 cm)
#define US_READ_INTERVAL    40      // ms between ultrasonic reads

// — Button --------------------------------------------------------------
#define BTN_PIN             6       // Start / Reset button (active LOW)
#define BTN_DEBOUNCE_MS     50      // Debounce window
#define BTN_LONG_PRESS_MS   5000    // Hold duration for long-press reset

// — Buzzer --------------------------------------------------------------
#define BUZZER_PIN          7
#define BUZZ_FREQ           1000    // Hz
#define BUZZ_DURATION       50      // ms

// — LCD 2004 I2C ---------------------------------------------------------
#define LCD_ADDR            0x27    // Common I2C address for PCF8574 adapter
#define LCD_COLS            20
#define LCD_ROWS            4

// — Gameplay -------------------------------------------------------------
#define WIN_SCORE           5       // First to this score wins
#define PADDLE_HEIGHT       16      // 1 column × 16 rows (full height)

// — Ball speed (pixels / second) ----------------------------------------
#define BALL_SPEED_INITIAL  8.0f    // Starting speed (px/s)
#define BALL_SPEED_MAX      60.0f   // Absolute max speed (px/s)
#define BALL_SPEED_MULT     1.15f   // Exponential multiplier per paddle bounce

// — Ball spawn angle range -----------------------------------------------
#define BALL_ANGLE_MIN      20      // Min vertical angle (degrees from horiz.)
#define BALL_ANGLE_MAX      60      // Max vertical angle

// — Refresh rates --------------------------------------------------------
#define DISPLAY_INTERVAL    20      // ms — matrix refresh (~50 Hz)
#define LCD_INTERVAL        250     // ms — LCD refresh (~4 Hz)
#define GAME_TICK_INTERVAL  10      // ms — physics tick (100 Hz)

// ============================================================================
// ██  GLOBAL OBJECTS
// ============================================================================

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// ============================================================================
// ██  GAME STATE MACHINE
// ============================================================================

enum GameState { IDLE, PLAYING, GAME_OVER };
GameState gameState = IDLE;

// ============================================================================
// ██  GAME VARIABLES
// ============================================================================

// Ball position & velocity (sub-pixel floats)
float ballX, ballY;
float ballVX, ballVY;
float ballSpeed;

// Paddle presence detected by ultrasonic sensors
bool paddle1Active, paddle2Active;

// Scores
uint8_t score1, score2;

// Framebuffer — 1 bit per pixel, row-major
// fb[row][byte]: DISP_H rows, each COLS_PER_ROW bytes (one byte = 8 columns)
uint8_t fb[DISP_H][COLS_PER_ROW];

// Timing
unsigned long lastDisplay   = 0;
unsigned long lastLCD       = 0;
unsigned long lastGameTick  = 0;
unsigned long lastUS        = 0;
unsigned long gameStartMs   = 0;

// Buzzer state
unsigned long buzzEndMs = 0;

// Game-over LCD draw flag (reset each time GAME_OVER is entered)
bool goLcdDone = false;

// Button state
uint8_t       btnState        = HIGH;
uint8_t       btnLastState    = HIGH;
unsigned long btnPressMs      = 0;
unsigned long btnLastDebounce = 0;

// Ultrasonic alternating sensor index (0 = P1, 1 = P2)
uint8_t usIndex = 0;

// ============================================================================
// ██  MAX7219 DRIVER
// ============================================================================

#define MAX_REG_NOOP        0x00
#define MAX_REG_DIGIT0      0x01
#define MAX_REG_DECODEMODE  0x09
#define MAX_REG_INTENSITY   0x0A
#define MAX_REG_SCANLIMIT   0x0B
#define MAX_REG_SHUTDOWN    0x0C
#define MAX_REG_DISPLAYTEST 0x0F

// Send one register write to all NUM_MODULES modules (same reg + data for all)
void maxWriteAll(uint8_t reg, uint8_t data) {
  digitalWrite(MAX_CS_PIN, LOW);
  for (uint8_t i = 0; i < NUM_MODULES; i++) {
    SPI.transfer(reg);
    SPI.transfer(data);
  }
  digitalWrite(MAX_CS_PIN, HIGH);
}

// Send per-module data: data[0] = last module in chain, data[count-1] = first
void maxWriteEach(uint8_t reg, uint8_t data[], uint8_t count) {
  digitalWrite(MAX_CS_PIN, LOW);
  for (int8_t i = (int8_t)count - 1; i >= 0; i--) {
    SPI.transfer(reg);
    SPI.transfer(data[i]);
  }
  digitalWrite(MAX_CS_PIN, HIGH);
}

void maxInit() {
  pinMode(MAX_CS_PIN, OUTPUT);
  digitalWrite(MAX_CS_PIN, HIGH);
  SPI.begin();
  // MAX7219 is the sole SPI device; the transaction is opened once and kept
  // open for the lifetime of the sketch (exclusive bus ownership).
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  maxWriteAll(MAX_REG_DISPLAYTEST, 0);  // Normal operation
  maxWriteAll(MAX_REG_SCANLIMIT,   7);  // Scan all 8 rows
  maxWriteAll(MAX_REG_DECODEMODE,  0);  // No BCD decode
  maxWriteAll(MAX_REG_INTENSITY,   1);  // Low brightness (0–15)
  maxWriteAll(MAX_REG_SHUTDOWN,    1);  // Normal operation (not shutdown)
}

// ============================================================================
// ██  FRAMEBUFFER HELPERS
// ============================================================================

void fbClear() {
  memset(fb, 0, sizeof(fb));
}

// Set or clear pixel (x = column 0..DISP_W-1, y = row 0..DISP_H-1)
void fbSet(int8_t x, int8_t y, bool on) {
  if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
  uint8_t byteIdx = x / 8;
  uint8_t bitIdx  = 7 - (x % 8);
  if (on) fb[y][byteIdx] |=  (1 << bitIdx);
  else    fb[y][byteIdx] &= ~(1 << bitIdx);
}

// Push framebuffer to MAX7219 modules
// Physical layout:
//   Module row 0 (y 0..7):  modules 0..3 left-to-right
//   Module row 1 (y 8..15): modules 4..7 left-to-right
void flushFB() {
  for (uint8_t row = 0; row < 8; row++) {
    uint8_t d[NUM_MODULES];
    // Module row 0 (top half): frame rows 0..7
    for (uint8_t col = 0; col < COLS_PER_ROW; col++) {
      d[col] = fb[row][col];
    }
    // Module row 1 (bottom half): frame rows 8..15
    for (uint8_t col = 0; col < COLS_PER_ROW; col++) {
      d[COLS_PER_ROW + col] = fb[row + 8][col];
    }
    maxWriteEach(MAX_REG_DIGIT0 + row, d, NUM_MODULES);
  }
}

// ============================================================================
// ██  BUTTON HANDLING
// ============================================================================

enum BtnEvent { BTN_NONE, BTN_SHORT, BTN_LONG };

BtnEvent btnRead() {
  BtnEvent ev    = BTN_NONE;
  uint8_t reading = digitalRead(BTN_PIN);
  unsigned long now = millis();

  if (reading != btnLastState) {
    btnLastDebounce = now;
  }
  btnLastState = reading;

  if ((now - btnLastDebounce) > BTN_DEBOUNCE_MS) {
    if (reading != btnState) {
      btnState = reading;
      if (btnState == LOW) {
        btnPressMs = now;
      } else {
        unsigned long held = now - btnPressMs;
        ev = (held >= BTN_LONG_PRESS_MS) ? BTN_LONG : BTN_SHORT;
      }
    }
  }
  return ev;
}

// ============================================================================
// ██  BUZZER
// ============================================================================

void buzzTone() {
  tone(BUZZER_PIN, BUZZ_FREQ);
  buzzEndMs = millis() + BUZZ_DURATION;
}

void buzzUpdate() {
  if (buzzEndMs > 0 && millis() >= buzzEndMs) {
    noTone(BUZZER_PIN);
    buzzEndMs = 0;
  }
}

// ============================================================================
// ██  ULTRASONIC SENSORS
// ============================================================================

// Non-blocking: alternate between P1 and P2 each call
// Note: the HC-SR04 trigger pulse requires a 10 µs blocking delay (~12 µs total);
// this is unavoidable per the sensor datasheet and has negligible impact on timing.
void usUpdate() {
  uint8_t trigPin = (usIndex == 0) ? US1_TRIG_PIN : US2_TRIG_PIN;
  uint8_t echoPin = (usIndex == 0) ? US1_ECHO_PIN : US2_ECHO_PIN;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long dur = pulseIn(echoPin, HIGH, US_TIMEOUT_US);
  float distCm      = dur * 0.01715f;  // µs → cm (343 m/s, round trip)

  bool active = (dur > 0 && distCm < PADDLE_DIST_CM);
  if (usIndex == 0) paddle1Active = active;
  else              paddle2Active = active;

  usIndex = 1 - usIndex;
}

// ============================================================================
// ██  LCD HELPERS  (forward declarations needed by gameTick)
// ============================================================================

void lcdShowIdle();
void lcdShowPlaying();
void lcdShowGameOver();

// ============================================================================
// ██  GAME LOGIC
// ============================================================================

// Spawn ball from centre toward the indicated player's side
void spawnBall(bool toPlayer1) {
  ballX     = DISP_W / 2.0f;
  ballY     = DISP_H / 2.0f;
  ballSpeed = BALL_SPEED_INITIAL;

  float angleDeg = BALL_ANGLE_MIN + random(BALL_ANGLE_MAX - BALL_ANGLE_MIN + 1);
  float angleRad = angleDeg * (PI / 180.0f);
  ballVX = ballSpeed * cos(angleRad) * (toPlayer1 ? -1.0f : 1.0f);
  ballVY = ballSpeed * sin(angleRad) * (random(2) ? 1.0f : -1.0f);
}

void gameReset() {
  score1        = 0;
  score2        = 0;
  paddle1Active = false;
  paddle2Active = false;
  spawnBall((bool)random(2));
}

// Physics tick — called every GAME_TICK_INTERVAL ms
void gameTick(float dt) {
  ballX += ballVX * dt;
  ballY += ballVY * dt;

  // Top / bottom wall bounce
  if (ballY < 0.0f) {
    ballY  = -ballY;
    ballVY = -ballVY;
    buzzTone();
  }
  if (ballY >= (float)DISP_H) {
    ballY  = 2.0f * (float)DISP_H - ballY;
    ballVY = -ballVY;
    buzzTone();
  }

  // Left paddle (x == 0) — Player 1
  if (ballVX < 0.0f && ballX <= 1.0f) {
    if (paddle1Active) {
      ballSpeed = min(ballSpeed * BALL_SPEED_MULT, BALL_SPEED_MAX);
      float angle = (BALL_ANGLE_MIN + random(BALL_ANGLE_MAX - BALL_ANGLE_MIN + 1)) * (PI / 180.0f);
      ballVX =  ballSpeed * cos(angle);
      ballVY =  ballSpeed * sin(angle) * (ballVY < 0.0f ? -1.0f : 1.0f);
      ballX  = 1.0f;
      buzzTone();
    } else {
      score2++;
      buzzTone();
      if (score2 >= WIN_SCORE) {
        gameState  = GAME_OVER;
        goLcdDone  = false;
        lcdShowGameOver();
        return;
      }
      spawnBall(true);
    }
  }

  // Right paddle (x == DISP_W-1) — Player 2
  if (ballVX > 0.0f && ballX >= (float)(DISP_W - 1)) {
    if (paddle2Active) {
      ballSpeed = min(ballSpeed * BALL_SPEED_MULT, BALL_SPEED_MAX);
      float angle = (BALL_ANGLE_MIN + random(BALL_ANGLE_MAX - BALL_ANGLE_MIN + 1)) * (PI / 180.0f);
      ballVX = -ballSpeed * cos(angle);
      ballVY =  ballSpeed * sin(angle) * (ballVY < 0.0f ? -1.0f : 1.0f);
      ballX  = (float)(DISP_W - 2);
      buzzTone();
    } else {
      score1++;
      buzzTone();
      if (score1 >= WIN_SCORE) {
        gameState  = GAME_OVER;
        goLcdDone  = false;
        lcdShowGameOver();
        return;
      }
      spawnBall(false);
    }
  }
}

// ============================================================================
// ██  RENDER FUNCTIONS
// ============================================================================

void renderIdle() {
  // Attract-mode: bouncing ball + paddles
  static float          ix          = DISP_W / 2.0f;
  static float          iy          = DISP_H / 2.0f;
  static float          ivx         = 4.0f;
  static float          ivy         = 3.0f;
  static unsigned long  lastIdleTick = 0;

  unsigned long now = millis();
  float dt = (float)(now - lastIdleTick) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f;
  lastIdleTick = now;

  ix += ivx * dt;
  iy += ivy * dt;
  if (ix <= 0.0f || ix >= (float)(DISP_W - 1)) { ivx = -ivx; ix = constrain(ix, 0.0f, (float)(DISP_W - 1)); }
  if (iy <= 0.0f || iy >= (float)(DISP_H - 1)) { ivy = -ivy; iy = constrain(iy, 0.0f, (float)(DISP_H - 1)); }

  fbClear();
  fbSet((int8_t)ix, (int8_t)iy, true);
  for (uint8_t y = 0; y < DISP_H; y++) {
    fbSet(0,          y, true);
    fbSet(DISP_W - 1, y, true);
  }
  flushFB();
}

void renderPlaying() {
  fbClear();
  fbSet((int8_t)ballX, (int8_t)ballY, true);
  if (paddle1Active) {
    for (uint8_t y = 0; y < DISP_H; y++) fbSet(0,          y, true);
  }
  if (paddle2Active) {
    for (uint8_t y = 0; y < DISP_H; y++) fbSet(DISP_W - 1, y, true);
  }
  // Centre dotted line
  for (uint8_t y = 0; y < DISP_H; y += 2) fbSet(DISP_W / 2, y, true);
  flushFB();
}

void renderGameOver() {
  static unsigned long lastBlink = 0;
  static bool          blinkOn   = true;

  if (millis() - lastBlink >= 300) {
    blinkOn  = !blinkOn;
    lastBlink = millis();
  }
  fbClear();
  if (blinkOn) {
    for (uint8_t y = 0; y < DISP_H; y++)
      for (uint8_t x = 0; x < DISP_W; x++)
        fbSet(x, y, true);
  }
  flushFB();
}

// ============================================================================
// ██  LCD HELPERS
// ============================================================================

void lcdShowIdle() {
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print(F("PONG  -  READY"));
  lcd.setCursor(1, 2);
  lcd.print(F("Press BTN to start"));
}

void lcdShowPlaying() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("P1: "));
  lcd.print(score1);
  lcd.setCursor(15, 0);
  lcd.print(F("P2: "));
  lcd.print(score2);
  lcd.setCursor(5, 2);
  lcd.print(F("PLAYING..."));
}

void lcdShowGameOver() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(F("** GAME OVER **"));
  lcd.setCursor(0, 1);
  if (score1 >= WIN_SCORE) lcd.print(F("   Player 1 WINS!"));
  else                      lcd.print(F("   Player 2 WINS!"));
  lcd.setCursor(0, 2);
  lcd.print(F("  P1:"));
  lcd.print(score1);
  lcd.print(F("   P2:"));
  lcd.print(score2);
  lcd.setCursor(1, 3);
  lcd.print(F("Press BTN to reset"));
}

// ============================================================================
// ██  SETUP
// ============================================================================

void setup() {
  // Pins
  pinMode(BTN_PIN,      INPUT_PULLUP);
  pinMode(BUZZER_PIN,   OUTPUT);
  pinMode(US1_TRIG_PIN, OUTPUT);
  pinMode(US1_ECHO_PIN, INPUT);
  pinMode(US2_TRIG_PIN, OUTPUT);
  pinMode(US2_ECHO_PIN, INPUT);

  // Random seed from floating analog pin
  randomSeed(analogRead(A0));

  // MAX7219
  maxInit();
  fbClear();
  flushFB();

  // LCD
  lcd.init();
  lcd.backlight();
  lcdShowIdle();

  // Initial state
  gameState = IDLE;
}

// ============================================================================
// ██  MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();

  // ── Always: buzzer update ─────────────────────────────────────────────
  buzzUpdate();

  // ── Always: button read ───────────────────────────────────────────────
  BtnEvent btn = btnRead();

  // ── Always: ultrasonic sensors (alternating) ──────────────────────────
  if (now - lastUS >= US_READ_INTERVAL) {
    lastUS = now;
    usUpdate();
  }

  // ══════════════════════════════════════════════════════════════════════
  //  STATE MACHINE
  // ══════════════════════════════════════════════════════════════════════

  switch (gameState) {

    // ── IDLE ────────────────────────────────────────────────────────────
    case IDLE:
      if (btn == BTN_SHORT) {
        gameReset();
        gameStartMs = millis();
        gameState   = PLAYING;
        lcdShowPlaying();
      }
      // Render idle display
      if (now - lastDisplay >= DISPLAY_INTERVAL) {
        lastDisplay = now;
        renderIdle();
      }
      break;

    // ── PLAYING ─────────────────────────────────────────────────────────
    case PLAYING:
      // Long press → reset to IDLE
      if (btn == BTN_LONG) {
        gameState = IDLE;
        lcdShowIdle();
        break;
      }

      // Physics tick
      if (now - lastGameTick >= GAME_TICK_INTERVAL) {
        float dt = (float)(now - lastGameTick) / 1000.0f;
        lastGameTick = now;
        gameTick(dt);
      }

      // Matrix render
      if (now - lastDisplay >= DISPLAY_INTERVAL) {
        lastDisplay = now;
        renderPlaying();
      }

      // LCD update
      if (now - lastLCD >= LCD_INTERVAL) {
        lastLCD = now;
        lcdShowPlaying();
      }
      break;

    // ── GAME OVER ───────────────────────────────────────────────────────
    case GAME_OVER:
      // Any button press → back to IDLE
      if (btn == BTN_SHORT || btn == BTN_LONG) {
        gameState = IDLE;
        lcdShowIdle();
        break;
      }

      // Matrix render (blink effect)
      if (now - lastDisplay >= DISPLAY_INTERVAL) {
        lastDisplay = now;
        renderGameOver();
      }

      // LCD (show once per GAME_OVER entry to avoid flickering)
      if (now - lastLCD >= LCD_INTERVAL) {
        lastLCD = now;
        if (!goLcdDone) { lcdShowGameOver(); goLcdDone = true; }
      }
      break;
  }
}
