# Maquette 1 — PONG

Jeu Pong deux joueurs sur Arduino UNO avec un affichage matriciel LED 32×16,
commande par capteurs ultrasoniques, buzzer et écran LCD 20×4.

## Description

Chaque joueur déplace sa raquette en rapprochant ou en éloignant la main de
son capteur HC-SR04 (de 3 cm à 30 cm). La balle rebondit sur les parois
haute et basse, et s'accélère légèrement à chaque contact avec une raquette.
Le premier joueur à atteindre **7 points** remporte la partie.

Le jeu fonctionne selon une **machine à états non bloquante** basée
entièrement sur `millis()` (aucun `delay()`) :

| État | Description |
|---|---|
| `IDLE` | Titre affiché, attente du bouton |
| `COUNTDOWN` | Décompte 3 – 2 – 1 – GO ! |
| `PLAYING` | Jeu en cours |
| `POINT` | Flash après un point marqué |
| `GAMEOVER` | Affichage du vainqueur, attente du bouton pour rejouer |

## Composants utilisés

| Composant | Référence | Quantité |
|---|---|---|
| Microcontrôleur | Arduino UNO (ATmega328P) | 1 |
| Matrice LED 8×8 | Module MAX7219 | 8 |
| Capteur ultrasonique | HC-SR04 | 2 |
| Afficheur LCD | LCD 20×4 + module I2C (PCF8574) | 1 |
| Buzzer passif | 5 V | 1 |
| Bouton poussoir | Momentané, normalement ouvert | 1 |

## Structure du dossier

```
maquette-1/
├── code/
│   ├── pong.ino          # Code source Arduino (fichier principal)
│   └── bibliotheques.md  # Bibliothèques requises et procédure d'installation
├── schemas/
│   └── connexions.md     # Tableau de câblage complet
└── README.md             # Ce fichier
```

## Code

Le code source se trouve dans [`code/pong.ino`](./code/pong.ino).

Les bibliothèques à installer sont listées dans [`code/bibliotheques.md`](./code/bibliotheques.md).

## Schémas électriques

Le tableau de câblage détaillé est dans [`schemas/connexions.md`](./schemas/connexions.md).

## Instructions de montage

1. Installer les bibliothèques **LedControl** et **LiquidCrystal I2C** via le Gestionnaire de bibliothèques Arduino IDE (voir [`code/bibliotheques.md`](./code/bibliotheques.md)).
2. Câbler les 8 modules MAX7219 en daisy-chain sur D10/D11/D13.
   - Rangée haute : modules 0 à 3 (colonnes 0–31, lignes 0–7).
   - Rangée basse : modules 4 à 7 (colonnes 0–31, lignes 8–15).
3. Connecter les deux HC-SR04 sur A0/A1 (J1) et A2/A3 (J2).
4. Connecter le LCD I2C sur A4 (SDA) et A5 (SCL).
5. Connecter le buzzer sur D9 et le bouton sur D2.
6. Ouvrir `code/pong.ino` dans l'Arduino IDE, sélectionner la carte **Arduino Uno** et le bon port série, puis téléverser.
7. Appuyer sur le bouton pour lancer une partie.

> **Alimentation :** Avec 8 modules MAX7219 allumés, préférer une alimentation externe
> 5 V / 2 A plutôt que la seule alimentation USB.
