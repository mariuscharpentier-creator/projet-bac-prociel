#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int PIN_BOUTON = 7;
const int PIN_LED    = 10;
const int PIN_BUZZER = 2;
const unsigned long DEBOUNCE_MS = 50;

unsigned long tempsDepart      = 0;
unsigned long tempsReactionMs  = 0;
float         tempsReactionSec = 0.0;

void setup() {
  pinMode(PIN_BOUTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  lcd.init();
  lcd.backlight();
  randomSeed(analogRead(A0));
}

bool isButtonStable(int expected) {
  if (digitalRead(PIN_BOUTON) != expected) return false;
  unsigned long start = millis();
  while (millis() - start < DEBOUNCE_MS) {
    if (digitalRead(PIN_BOUTON) != expected) return false;
  }
  return true;
}

void waitForPress()   { while (!isButtonStable(LOW))  {} }
void waitForRelease() { while (!isButtonStable(HIGH)) {} }

void loop() {
  afficherMessageDepart();
  waitForPress();
  delay(20);
  waitForRelease();
  afficherAttente();
  attendreAleatoire();
  waitForRelease();
  digitalWrite(PIN_LED, HIGH);
  tone(PIN_BUZZER, 4000);
  tempsDepart = millis();
  waitForPress();
  tempsReactionMs  = millis() - tempsDepart;
  tempsReactionSec = tempsReactionMs / 1000.0;
  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);
  waitForRelease();
  afficherResultat();
  attendreNouvellePartie();
}

void afficherMessageDepart() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Pret ?");
  lcd.setCursor(0, 1); lcd.print("Appuie bouton");
}

void afficherAttente() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Attends...");
  lcd.setCursor(0, 1); lcd.print("Ne touche pas");
}

void attendreAleatoire() {
  int attente = random(1000, 7001);
  unsigned long debut = millis();
  while (millis() - debut < (unsigned long)attente) {}
}

void afficherResultat() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Temps reaction");
  lcd.setCursor(0, 1);
  lcd.print(tempsReactionSec, 2);
  lcd.print(" s");
  delay(4000);
}

void attendreNouvellePartie() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Rejouer ?");
  lcd.setCursor(0, 1); lcd.print("Appuie bouton");
  waitForPress();
  waitForRelease();
}
