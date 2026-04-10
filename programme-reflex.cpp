#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -----------------------------------------------------------------------------
// Projet : Reflex - Bac Pro CIEL
// Carte : Arduino Nano
//
// Description :
// Ce programme permet de mesurer le temps de réaction d'un utilisateur.
// Au démarrage, un message s'affiche sur l'écran LCD.
// L'utilisateur doit appuyer sur le bouton pour lancer le test.
// Ensuite, après un délai aléatoire entre 3000 ms et 5000 ms,
// la LED s'allume et le buzzer sonne.
// L'utilisateur doit alors appuyer le plus vite possible sur le bouton.
// Le temps de réaction est ensuite affiché en millisecondes sur l'écran.
// -----------------------------------------------------------------------------

// Création de l'objet LCD : adresse 0x27, écran 16 colonnes x 2 lignes
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -----------------------------------------------------------------------------
// Définition des broches
// -----------------------------------------------------------------------------
const int PIN_BOUTON = 7;   // bouton poussoir
const int PIN_LED = 1;      // LED
const int PIN_BUZZER = 2;   // buzzer

// Variables pour mesurer le temps
unsigned long tempsDepart = 0;
unsigned long tempsReaction = 0;

// -----------------------------------------------------------------------------
// Fonction setup() : exécutée une seule fois au démarrage
// -----------------------------------------------------------------------------
void setup() {
  // Configuration des broches
  pinMode(PIN_BOUTON, INPUT_PULLUP); // bouton actif à l'état bas
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // On éteint la LED et le buzzer au démarrage
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  // Initialisation de l'écran LCD
  lcd.init();
  lcd.backlight();

  // Initialisation d'une graine aléatoire
  // Cela permet de ne pas avoir toujours le même temps d'attente
  randomSeed(analogRead(A0));
}

// -----------------------------------------------------------------------------
// Boucle principale
// -----------------------------------------------------------------------------
void loop() {
  afficherMessageDepart();
  attendreAppuiPourDemarrer();
  lancerTestReflexe();
  afficherResultat();
  attendreAvantNouvellePartie();
}

// -----------------------------------------------------------------------------
// Affiche le message de départ sur l'écran LCD
// -----------------------------------------------------------------------------
void afficherMessageDepart() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pret ?");
  lcd.setCursor(0, 1);
  lcd.print("Alors appuie");
}

// -----------------------------------------------------------------------------
// Attend que l'utilisateur appuie sur le bouton pour lancer le test
// -----------------------------------------------------------------------------
void attendreAppuiPourDemarrer() {
  while (digitalRead(PIN_BOUTON) == HIGH) {
    // On attend sans rien faire
  }

  // Anti-rebond
  delay(50);

  while (digitalRead(PIN_BOUTON) == LOW) {
    // On attend le relâchement
  }

  delay(50);
}

// -----------------------------------------------------------------------------
// Lance le test de réflexe
// -----------------------------------------------------------------------------
void lancerTestReflexe() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Attends...");
  lcd.setCursor(0, 1);
  lcd.print("Signal bientot");

  // Délai aléatoire entre 3000 ms et 5000 ms
  int attenteAleatoire = random(3000, 5001);
  delay(attenteAleatoire);

  // Activation de la LED et du buzzer
  digitalWrite(PIN_LED, HIGH);
  tone(PIN_BUZZER, 4000);

  // On mémorise le départ en millisecondes
  tempsDepart = millis();

  // Attente de l'appui sur le bouton
  while (digitalRead(PIN_BOUTON) == HIGH) {
    // On attend
  }

  // Calcul du temps de réaction en millisecondes
  tempsReaction = millis() - tempsDepart;

  // On coupe la LED et le buzzer
  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);

  // Anti-rebond
  delay(50);

  while (digitalRead(PIN_BOUTON) == LOW) {
    // On attend le relâchement
  }

  delay(50);
}

// -----------------------------------------------------------------------------
// Affiche le temps de réaction sur l'écran LCD
// -----------------------------------------------------------------------------
void afficherResultat() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temps reaction");

  lcd.setCursor(0, 1);
  lcd.print(tempsReaction);
  lcd.print(" ms");

  delay(4000);
}

// -----------------------------------------------------------------------------
// Attend avant de recommencer une nouvelle partie
// -----------------------------------------------------------------------------
void attendreAvantNouvellePartie() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rejouer ?");
  lcd.setCursor(0, 1);
  lcd.print("Appuie bouton");

  while (digitalRead(PIN_BOUTON) == HIGH) {
    // Attente
  }

  delay(50);

  while (digitalRead(PIN_BOUTON) == LOW) {
    // Attente
  }

  delay(50);
}
