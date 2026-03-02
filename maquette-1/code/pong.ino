// ============================================================================
//  PONG GAME — Arduino UNO
//  8x MAX7219 (2 rows × 4 cols = 32×16), 2× HC-SR04, Button, Buzzer, LCD2004
//  Architecture : State-machine  ·  Non-blocking  ·  millis() only
// ============================================================================

// ────────────────────────── LIBRARIES ───────────────────────────────────────
#include <LedControl.h>          // MAX7219 LED matrix driver
#include <LiquidCrystal_I2C.h>  // LCD 20×4 with I2C backpack

// ────────────────────────── PIN DEFINITIONS ─────────────────────────────────
// MAX7219 — daisy-chain of 8 devices (hardware SPI pins on UNO)
#define PIN_DIN    11
#define PIN_CLK    13
#define PIN_CS     10

// HC-SR04 — Player 1 (left paddle)
#define PIN_TRIG1   A0
#define PIN_ECHO1   A1

// HC-SR04 — Player 2 (right paddle)
#define PIN_TRIG2   A2
#define PIN_ECHO2   A3

// Push-button (active-LOW, internal pull-up enabled)
#define PIN_BUTTON   2

// Passive buzzer
#define PIN_BUZZER   9

// ────────────────────────── DISPLAY CONSTANTS ────────────────────────────────
#define NUM_DEVICES    8    // 8 MAX7219 in one daisy-chain
#define DISPLAY_W     32    // total columns  (4 matrices × 8 columns)
#define DISPLAY_H     16    // total rows     (2 matrices × 8 rows)

// ────────────────────────── GAME CONSTANTS ───────────────────────────────────
#define PADDLE_LEN       4    // paddle height in pixels
#define SCORE_WIN        7    // first player to reach this score wins

#define SONAR_MIN_CM     3    // minimum readable distance (cm)
#define SONAR_MAX_CM    30    // maximum readable distance (cm)

// Timing (milliseconds)
#define SONAR_PERIOD      50U   // sonar poll interval
#define BALL_PERIOD_INIT 160U   // initial ball step interval
#define BALL_PERIOD_MIN   60U   // fastest ball step interval
#define BALL_ACCEL         5U   // period decrease per paddle hit
#define DISPLAY_PERIOD    16U   // ~60 fps display refresh
#define BLINK_PERIOD     400U   // idle / game-over blink rate
#define POINT_PAUSE     1200U   // pause after each point (flash)
#define COUNTDOWN_STEP  1000U   // 1 s per countdown digit

// Tone frequencies (Hz)
#define TONE_PADDLE   440
#define TONE_WALL     220
#define TONE_SCORE    880
#define TONE_GAMEOVER 110
#define TONE_BEEP     600

// Tone durations (ms)
#define DUR_SHORT  60
#define DUR_BEEP  100
#define DUR_SCORE 300
#define DUR_END   800

// ────────────────────────── OBJECTS ──────────────────────────────────────────
// LedControl(dataPin, clkPin, csPin, numDevices)
LedControl lc(PIN_DIN, PIN_CLK, PIN_CS, NUM_DEVICES);

// LCD 20×4 at I2C address 0x27 (use 0x3F if display stays blank)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ────────────────────────── STATE MACHINE ────────────────────────────────────
enum GameState : uint8_t {
  STATE_IDLE,
  STATE_COUNTDOWN,
  STATE_PLAYING,
  STATE_POINT,
  STATE_GAMEOVER
};
static GameState gState = STATE_IDLE;

// ────────────────────────── GAME VARIABLES ───────────────────────────────────
static float    ballX, ballY;         // ball position (float for smooth physics)
static float    ballDX, ballDY;       // ball direction (±1.0)
static int      paddle1Y;             // top pixel of left  paddle
static int      paddle2Y;             // top pixel of right paddle
static uint8_t  score1, score2;       // player scores
static uint8_t  countdown;            // 3 … 0
static unsigned long ballPeriod;      // current ball step interval

// ────────────────────────── TIMERS ───────────────────────────────────────────
static unsigned long tSonar;
static unsigned long tBall;
static unsigned long tDisplay;
static unsigned long tBlink;
static unsigned long tPoint;
static unsigned long tCountdown;

// ────────────────────────── FRAME BUFFER ─────────────────────────────────────
// 16 rows × 32 bits — bit N = column N (0 = left, 31 = right)
static uint32_t frame[DISPLAY_H];

// ============================================================================
//  DISPLAY HELPERS
// ============================================================================

static void frameClear() {
  memset(frame, 0, sizeof(frame));
}

static void frameSetPixel(int x, int y, bool on) {
  if (x < 0 || x >= DISPLAY_W || y < 0 || y >= DISPLAY_H) return;
  if (on) frame[y] |=  (1UL << x);
  else    frame[y] &= ~(1UL << x);
}

// Push frame buffer to the 8 MAX7219 devices.
//
// Physical wiring layout (daisy-chain order 0 → 7):
//   Devices 0-3 : top    strip (rows  0-7 ),  left-to-right
//   Devices 4-7 : bottom strip (rows 8-15 ),  left-to-right
//
// Each device owns 8 columns.  Column mapping:
//   device 0 → cols  0- 7
//   device 1 → cols  8-15
//   device 2 → cols 16-23
//   device 3 → cols 24-31  (same pattern repeated for devices 4-7)
//
static void framePush() {
  for (int y = 0; y < DISPLAY_H; y++) {
    uint32_t row    = frame[y];
    int      devRow = y >> 3;          // 0 = top strip, 1 = bottom strip
    int      regRow = y &  7;          // row register inside the device (0-7)
    int      devBase = devRow * 4;     // first device index of this strip
    for (int d = 0; d < 4; d++) {
      uint8_t cols = (uint8_t)((row >> (d * 8)) & 0xFF);
      lc.setRow(devBase + d, regRow, cols);
    }
  }
}

static void drawPaddle(int x, int topY) {
  for (int i = 0; i < PADDLE_LEN; i++)
    frameSetPixel(x, topY + i, true);
}

static void drawBorders() {
  for (int x = 0; x < DISPLAY_W; x++) {
    frameSetPixel(x, 0,            true);
    frameSetPixel(x, DISPLAY_H-1,  true);
  }
  for (int y = 0; y < DISPLAY_H; y++) {
    frameSetPixel(0,           y, true);
    frameSetPixel(DISPLAY_W-1, y, true);
  }
}

static void fillDisplay(bool on) {
  uint32_t v = on ? 0xFFFFFFFFUL : 0UL;
  for (int y = 0; y < DISPLAY_H; y++) frame[y] = v;
}

// ============================================================================
//  LCD HELPERS
// ============================================================================

static void lcdShowIdle() {
  lcd.clear();
  lcd.setCursor(5,  0); lcd.print(F("PONG GAME"));
  lcd.setCursor(0,  1); lcd.print(F("Appuyez sur bouton  "));
  lcd.setCursor(3,  2); lcd.print(F("pour demarrer"));
  lcd.setCursor(1,  3); lcd.print(F("BAC PRO CIEL - 2025"));
}

static void lcdShowCountdown(int n) {
  lcd.setCursor(9, 1);
  if (n > 0) {
    lcd.print(n);
    lcd.print(' ');
  } else {
    lcd.print(F("GO!"));
  }
}

static void lcdShowScore() {
  lcd.setCursor(0, 0);
  lcd.print(F("J1: "));
  lcd.print(score1);
  lcd.print(F("       J2: "));
  lcd.print(score2);
  lcd.print(F("  "));
}

static void lcdShowGameover() {
  lcd.clear();
  lcd.setCursor(2, 0);
  if (score1 >= SCORE_WIN)
    lcd.print(F("JOUEUR 1 GAGNE !"));
  else
    lcd.print(F("JOUEUR 2 GAGNE !"));
  lcd.setCursor(0, 1);
  lcd.print(F("Score final: "));
  lcd.print(score1); lcd.print(F(" - ")); lcd.print(score2);
  lcd.setCursor(0, 3);
  lcd.print(F("Bouton = rejouer    "));
}

// ============================================================================
//  SOUND
// ============================================================================

static void beep(unsigned int freq, unsigned int durMs) {
  tone(PIN_BUZZER, freq, durMs);
}

// ============================================================================
//  BUTTON (edge-detect, active-LOW)
// ============================================================================

static bool buttonPressed() {
  static bool last = HIGH;
  bool cur     = digitalRead(PIN_BUTTON);
  bool pressed = (last == HIGH && cur == LOW);
  last = cur;
  return pressed;
}

// ============================================================================
//  SONAR
// ============================================================================

// Single blocking pulse (≤ ~30 ms worst case). Returns cm or -1 on timeout.
static int readSonar(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long us = pulseIn(echoPin, HIGH, 30000UL);
  if (us == 0) return -1;
  return (int)(us / 58L);
}

// Map distance to paddle top-Y, clamped to valid range
static int sonarToPaddleY(int cm) {
  cm = constrain(cm, SONAR_MIN_CM, SONAR_MAX_CM);
  return map(cm, SONAR_MIN_CM, SONAR_MAX_CM, 0, DISPLAY_H - PADDLE_LEN);
}

// ============================================================================
//  GAME LOGIC
// ============================================================================

static void resetBall(int dirX) {
  ballX  = DISPLAY_W / 2.0f;
  ballY  = DISPLAY_H / 2.0f;
  ballDX = (dirX >= 0) ?  1.0f : -1.0f;
  ballDY = (random(2) == 0) ?  1.0f : -1.0f;
  ballPeriod = BALL_PERIOD_INIT;
}

static void startGame() {
  score1   = 0;
  score2   = 0;
  paddle1Y = (DISPLAY_H - PADDLE_LEN) / 2;
  paddle2Y = (DISPLAY_H - PADDLE_LEN) / 2;
  resetBall(1);
  lcd.clear();
  lcdShowScore();
}

static void enterCountdown() {
  gState        = STATE_COUNTDOWN;
  countdown     = 3;
  tCountdown    = millis();
  lcd.clear();
  lcd.setCursor(3, 1); lcd.print(F("Pret dans..."));
  lcdShowCountdown(3);
  beep(TONE_BEEP, DUR_BEEP);
}

// Called every ballPeriod ms while STATE_PLAYING
static void updateBall() {
  ballX += ballDX;
  ballY += ballDY;

  // ── Bounce: top / bottom walls ──────────────────────────────────────────
  if (ballY < 0) {
    ballY  = 0;
    ballDY = 1.0f;
    beep(TONE_WALL, DUR_SHORT);
  }
  if (ballY >= DISPLAY_H) {
    ballY  = DISPLAY_H - 1;
    ballDY = -1.0f;
    beep(TONE_WALL, DUR_SHORT);
  }

  int by = (int)ballY;

  // ── Left side: paddle 1 or player 2 scores ──────────────────────────────
  if ((int)ballX <= 0) {
    if (by >= paddle1Y && by < paddle1Y + PADDLE_LEN) {
      ballX  = 1;
      ballDX = 1.0f;
      beep(TONE_PADDLE, DUR_SHORT);
      if (ballPeriod > BALL_PERIOD_MIN) ballPeriod -= BALL_ACCEL;
    } else {
      score2++;
      beep(TONE_SCORE, DUR_SCORE);
      lcdShowScore();
      gState    = STATE_POINT;
      tPoint    = millis();
      tBlink    = millis();
    }
    return;
  }

  // ── Right side: paddle 2 or player 1 scores ─────────────────────────────
  if ((int)ballX >= DISPLAY_W - 1) {
    if (by >= paddle2Y && by < paddle2Y + PADDLE_LEN) {
      ballX  = DISPLAY_W - 2;
      ballDX = -1.0f;
      beep(TONE_PADDLE, DUR_SHORT);
      if (ballPeriod > BALL_PERIOD_MIN) ballPeriod -= BALL_ACCEL;
    } else {
      score1++;
      beep(TONE_SCORE, DUR_SCORE);
      lcdShowScore();
      gState    = STATE_POINT;
      tPoint    = millis();
      tBlink    = millis();
    }
  }
}

// ============================================================================
//  SETUP
// ============================================================================

void setup() {
  // ── Pin modes ─────────────────────────────────────────────────────────────
  pinMode(PIN_TRIG1,  OUTPUT);
  pinMode(PIN_ECHO1,  INPUT);
  pinMode(PIN_TRIG2,  OUTPUT);
  pinMode(PIN_ECHO2,  INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  // ── MAX7219 init ──────────────────────────────────────────────────────────
  for (int i = 0; i < NUM_DEVICES; i++) {
    lc.shutdown(i, false);    // wake up from power-down mode
    lc.setIntensity(i, 4);    // brightness 0 (min) … 15 (max)
    lc.clearDisplay(i);
  }

  // ── LCD init ──────────────────────────────────────────────────────────────
  lcd.init();
  lcd.backlight();
  lcdShowIdle();

  randomSeed(analogRead(A5));

  // ── Init timers ───────────────────────────────────────────────────────────
  unsigned long now = millis();
  tSonar   = now;
  tBall    = now;
  tDisplay = now;
  tBlink   = now;

  gState = STATE_IDLE;
}

// ============================================================================
//  MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();

  // ── Sonar update (both sensors) ──────────────────────────────────────────
  if (now - tSonar >= SONAR_PERIOD) {
    tSonar = now;
    int cm1 = readSonar(PIN_TRIG1, PIN_ECHO1);
    int cm2 = readSonar(PIN_TRIG2, PIN_ECHO2);
    if (cm1 > 0) paddle1Y = sonarToPaddleY(cm1);
    if (cm2 > 0) paddle2Y = sonarToPaddleY(cm2);
  }

  // ── State machine ─────────────────────────────────────────────────────────
  switch (gState) {

    // ════════════════════════════════════════════════════════════════════════
    case STATE_IDLE:
    {
      // Blink border on display
      if (now - tBlink >= BLINK_PERIOD) {
        tBlink = now;
        static bool bOn = false;
        bOn = !bOn;
        frameClear();
        if (bOn) drawBorders();
        framePush();
      }

      if (buttonPressed()) {
        enterCountdown();
      }
      break;
    }

    // ════════════════════════════════════════════════════════════════════════
    case STATE_COUNTDOWN:
    {
      unsigned long elapsed = now - tCountdown;
      uint8_t step = (uint8_t)(elapsed / COUNTDOWN_STEP);

      // Update digit when a new second passes
      static uint8_t lastStep = 255;
      if (step != lastStep) {
        lastStep = step;
        if (step < 3) {
          countdown = 3 - step;
          lcdShowCountdown((int)countdown);
          beep(TONE_BEEP, DUR_BEEP);
        } else if (step == 3) {
          lcdShowCountdown(0);   // "GO!"
          beep(880, 200);
        }
      }

      // Show paddles during countdown
      if (now - tDisplay >= DISPLAY_PERIOD) {
        tDisplay = now;
        frameClear();
        drawPaddle(0,           paddle1Y);
        drawPaddle(DISPLAY_W-1, paddle2Y);
        framePush();
      }

      if (elapsed >= 4 * COUNTDOWN_STEP) {
        lastStep = 255;
        startGame();
        gState = STATE_PLAYING;
        tBall  = millis();
      }
      break;
    }

    // ════════════════════════════════════════════════════════════════════════
    case STATE_PLAYING:
    {
      // Ball step
      if (now - tBall >= ballPeriod) {
        tBall = now;
        updateBall();
      }

      // Display refresh
      if (now - tDisplay >= DISPLAY_PERIOD) {
        tDisplay = now;
        frameClear();
        drawPaddle(0,           paddle1Y);
        drawPaddle(DISPLAY_W-1, paddle2Y);
        frameSetPixel((int)ballX, (int)ballY, true);
        framePush();
      }
      break;
    }

    // ════════════════════════════════════════════════════════════════════════
    case STATE_POINT:
    {
      // Flash the whole display for POINT_PAUSE ms
      if (now - tBlink >= 150U) {
        tBlink = now;
        static bool fl = false;
        fl = !fl;
        frameClear();
        if (fl) fillDisplay(true);
        framePush();
      }

      if (now - tPoint >= POINT_PAUSE) {
        if (score1 >= SCORE_WIN || score2 >= SCORE_WIN) {
          gState = STATE_GAMEOVER;
          beep(TONE_GAMEOVER, DUR_END);
          lcdShowGameover();
          tBlink = millis();
        } else {
          // Serve toward the player who lost the point
          int dir = (score1 > score2) ? -1 : 1;
          resetBall(dir);
          gState = STATE_PLAYING;
          tBall  = millis();
          lcdShowScore();
        }
      }
      break;
    }

    // ════════════════════════════════════════════════════════════════════════
    case STATE_GAMEOVER:
    {
      // Blink winning side's column
      if (now - tBlink >= BLINK_PERIOD) {
        tBlink = now;
        static bool blW = false;
        blW = !blW;
        frameClear();
        if (blW) {
          int winX = (score1 >= SCORE_WIN) ? 0 : DISPLAY_W - 1;
          for (int y = 0; y < DISPLAY_H; y++)
            frameSetPixel(winX, y, true);
        }
        framePush();
      }

      if (buttonPressed()) {
        lcdShowIdle();
        gState = STATE_IDLE;
        tBlink = millis();
      }
      break;
    }
  } // end switch
}
