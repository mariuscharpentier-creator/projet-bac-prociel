# Bibliothèques Arduino requises — Maquette 1 PONG

Les deux bibliothèques suivantes doivent être installées via le **Gestionnaire de bibliothèques Arduino IDE**
(`Croquis → Inclure une bibliothèque → Gérer les bibliothèques…`).

| Bibliothèque | Auteur | Version testée | Utilisation |
|---|---|---|---|
| **LedControl** | Eberhard Fahle | ≥ 1.0.6 | Pilotage des matrices LED MAX7219 |
| **LiquidCrystal I2C** | Frank de Brabander | ≥ 1.1.2 | Afficheur LCD 20×4 via I2C |

## Installation

1. Ouvrir l'Arduino IDE.
2. Aller dans `Croquis → Inclure une bibliothèque → Gérer les bibliothèques…`.
3. Rechercher `LedControl` → sélectionner celle d'**Eberhard Fahle** → Installer.
4. Rechercher `LiquidCrystal I2C` → sélectionner celle de **Frank de Brabander** → Installer.
5. Ouvrir le fichier `pong.ino` et vérifier que la compilation ne signale pas d'erreur.

## Carte cible

- **Arduino UNO** (ATmega328P, 5 V, 16 MHz)
- Dans l'IDE : `Outils → Type de carte → Arduino AVR Boards → Arduino Uno`
