# Connexions électriques — Maquette 1 PONG

## Composants

| Composant | Référence | Quantité |
|---|---|---|
| Microcontrôleur | Arduino UNO (ATmega328P) | 1 |
| Matrice LED 8×8 | MAX7219 (module) | 8 |
| Capteur ultrasonique | HC-SR04 | 2 |
| Afficheur LCD | LCD 20×4 + module I2C (PCF8574) | 1 |
| Buzzer passif | 5 V | 1 |
| Bouton poussoir | momentané, normalement ouvert | 1 |
| Résistance | 10 kΩ (pull-down bouton si pas d'INPUT_PULLUP) | 1 |

---

## Matrice LED MAX7219 (8 modules en daisy-chain)

Les 8 modules MAX7219 sont câblés en **série** (sortie DOUT du module N vers DIN du module N+1).

```
Arduino UNO          MAX7219 n°1 (entrée de la chaîne)
─────────────        ──────────────────────────────────
D11  (MOSI)  ──────► DIN
D13  (SCK)   ──────► CLK
D10  (SS)    ──────► CS / LOAD
5 V          ──────► VCC
GND          ──────► GND
```

Chaînage :
```
MAX7219 n°1 DOUT ──► MAX7219 n°2 DIN ──► … ──► MAX7219 n°8 DIN
```

### Disposition physique des modules

```
┌──────┬──────┬──────┬──────┐  ← Rangée du haut  (dispositifs 0-3, lignes 0-7)
│  #0  │  #1  │  #2  │  #3  │    col 0-7  col 8-15  col 16-23  col 24-31
└──────┴──────┴──────┴──────┘
┌──────┬──────┬──────┬──────┐  ← Rangée du bas   (dispositifs 4-7, lignes 8-15)
│  #4  │  #5  │  #6  │  #7  │
└──────┴──────┴──────┴──────┘
```

Résolution totale : **32 colonnes × 16 lignes**

> **Note :** L'ordre dans la chaîne doit correspondre à l'ordre logique du code
> (dispositif 0 = coin haut-gauche, dispositif 7 = coin bas-droit, de gauche à droite).

---

## HC-SR04 — Capteur Joueur 1 (raquette gauche)

```
Arduino UNO     HC-SR04 J1
───────────     ──────────
A0  (TRIG1) ──► TRIG
A1  (ECHO1) ◄── ECHO
5 V         ──► VCC
GND         ──► GND
```

---

## HC-SR04 — Capteur Joueur 2 (raquette droite)

```
Arduino UNO     HC-SR04 J2
───────────     ──────────
A2  (TRIG2) ──► TRIG
A3  (ECHO2) ◄── ECHO
5 V         ──► VCC
GND         ──► GND
```

> Le joueur déplace sa raquette en rapprochant ou en éloignant sa main du capteur (3 cm → 30 cm).

---

## LCD 20×4 — Module I2C (PCF8574)

Le module I2C est directement soudé ou emboîté sur le LCD.

```
Arduino UNO     Module I2C / LCD
───────────     ────────────────
A4  (SDA)   ──► SDA
A5  (SCL)   ──► SCL
5 V         ──► VCC
GND         ──► GND
```

**Adresse I2C :** `0x27` (valeur par défaut). Si l'écran reste éteint, essayer `0x3F`
(modifier la ligne `LiquidCrystal_I2C lcd(0x27, 20, 4);` dans le code).

---

## Buzzer passif

```
Arduino UNO     Buzzer
───────────     ──────
D9          ──► + (broche positive)
GND         ──► − (broche négative / GND)
```

---

## Bouton poussoir

Le bouton utilise la **résistance de tirage interne** de l'Arduino (`INPUT_PULLUP`).
État logique : LOW = appuyé, HIGH = relâché.

```
Arduino UNO     Bouton
───────────     ──────
D2          ──── borne 1
GND         ──── borne 2
```

---

## Alimentation

| Source | Tension | Courant max estimé |
|---|---|---|
| USB (ordinateur) | 5 V | Suffisant pour développement |
| Adaptateur secteur DC jack | 7-12 V | Recommandé en utilisation finale |

> Avec 8 modules MAX7219 à luminosité maximale, la consommation peut dépasser 500 mA.
> En production, utiliser une alimentation externe 5 V / 2 A sur le jack DC ou sur la broche `VIN` + régulateur.

---

## Résumé des broches utilisées

| Broche Arduino | Fonction |
|---|---|
| D2 | Bouton poussoir |
| D9 | Buzzer passif |
| D10 | MAX7219 CS (Chip Select) |
| D11 | MAX7219 DIN (MOSI) |
| D13 | MAX7219 CLK (SCK) |
| A0 | HC-SR04 J1 — TRIG |
| A1 | HC-SR04 J1 — ECHO |
| A2 | HC-SR04 J2 — TRIG |
| A3 | HC-SR04 J2 — ECHO |
| A4 | LCD I2C — SDA |
| A5 | LCD I2C — SCL |
