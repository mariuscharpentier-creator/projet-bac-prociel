#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int PIN_BOUTON = 7;
const int PIN_LED = 10;
const int PIN_BUZZER = 2;

const unsigned long DEBOUNCE_MS = 50; // durée pour valider un état stable

unsigned long tempsDepart = 0;
unsigned long tempsReactionMs = 0;
float tempsReactionSec = 0.0;

void setup() {
  pinMode(PIN_BOUTON, INPUT_PULLUP); // bouton entre D7 et GND, press => LOW
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  lcd.init();
  lcd.backlight();

  randomSeed(analogRead(A0));
}

// Vérifie que la broche 'PIN_BOUTON' est à l'état 'expected' pendant DEBOUNCE_MS ms
bool isButtonStable(int expected) {
  if (digitalRead(PIN_BOUTON) != expected) return false;
  unsigned long start = millis();
  while (millis() - start < DEBOUNCE_MS) {
    if (digitalRead(PIN_BOUTON) != expected) return false;
  }
  return true;
}

// Attend un appui stable (pression -> LOW)
void waitForPress() {
  while (true) {
    if (isButtonStable(LOW)) return;
  }
}

// Attend un relâchement stable (HIGH)
void waitForRelease() {
  while (true) {
    if (isButtonStable(HIGH)) return;
  }
}

void loop() {
  afficherMessageDepart();

  // 1) Appui de départ puis relâchement validés
  waitForPress();
  delay(20); // petit délai supplémentaire pour sécurité
  waitForRelease();

  // 2) Attente aléatoire (pendant laquelle on ignore les appuis)
  afficherAttente();
  attendreAleatoire();

  // SÉCURITÉ : si quelqu'un maintient encore le bouton (appui intempestif), attendre le relâchement stable avant de signaler
  waitForRelease();

  // 3) Signal : LED + buzzer, on démarre le chrono
  digitalWrite(PIN_LED, HIGH);
  tone(PIN_BUZZER, 4000);

  tempsDepart = millis();

  // 4) Attendre l'appui de réaction (stable)
  waitForPress();

  // Calcul du temps
  tempsReactionMs = millis() - tempsDepart;
  tempsReactionSec = tempsReactionMs / 1000.0;

  // stop signal
  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);

  // 5) attendre le relâchement stable avant afficher
  waitForRelease();

  afficherResultat();

  // 6) Attendre nouvelle partie
  attendreNouvellePartie();
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
  lcd.print("Ne touche pas");
}

void attendreAleatoire() {
  int attente = random(1000, 7001);
  unsigned long debut = millis();
  while (millis() - debut < (unsigned long)attente) {
    // on pourrait surveiller ici pour debug, mais on ignore les appuis pendant l'attente
    // (si tu veux détecter "trop tôt", on peut modifier pour renvoyer un échec)
  }
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
  waitForPress();
  waitForRelease();
}
