#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int PIN_BOUTON = 7;
const int PIN_LED = 10;
const int PIN_BUZZER = 2;

unsigned long tempsDepart = 0;
unsigned long tempsReactionMs = 0;
float tempsReactionSec = 0.0;

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

void loop() {
  afficherMessageDepart();

  attendreAppui();
  attendreRelachement();

  afficherAttente();
  attendreAleatoire();

  // sécurité : vérifier que le bouton est bien relâché avant le signal
  while (digitalRead(PIN_BOUTON) == LOW) {
  }

  digitalWrite(PIN_LED, HIGH);
  tone(PIN_BUZZER, 4000);

  tempsDepart = millis();

  attendreAppui();

  tempsReactionMs = millis() - tempsDepart;
  tempsReactionSec = tempsReactionMs / 1000.0;

  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);

  attendreRelachement();

  afficherResultat();

  attendreNouvellePartie();
}

void attendreAppui() {
  while (digitalRead(PIN_BOUTON) == HIGH) {
  }
  delay(80);
}

void attendreRelachement() {
  while (digitalRead(PIN_BOUTON) == LOW) {
  }
  delay(80);
}

void afficherMessageDepart() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pret ?");
  lcd.setCursor(0, 1);
  lcd.print("Appuie bouton");
}

void afficherAttente() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Attends...");
  lcd.setCursor(0, 1);
  lcd.print("Signal bientot");
}

void attendreAleatoire() {
  int attente = random(3000, 5001);
  delay(attente);
}

void afficherResultat() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temps reaction");
  lcd.setCursor(0, 1);
  lcd.print(tempsReactionSec, 2);
  lcd.print(" s");

  delay(4000);
}

void attendreNouvellePartie() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rejouer ?");
  lcd.setCursor(0, 1);
  lcd.print("Appuie bouton");

  attendreAppui();
  attendreRelachement();
}
