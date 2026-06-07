# 🔌 Maquette Pédagogique — Bac Pro CIEL

> Projet de fin de formation réalisé dans le cadre du **Bac Pro CIEL**  
> (Cybersécurité, Informatique et réseaux, Électronique)  
> 4 circuits électroniques programmés en **C++ Arduino**

---

## 👥 Équipe

| Élève | Rôle |
|-------|------|
| Ozwan Le Priol | Développeur / Électronicien |
| Enzo Bermond | Développeur / Électronicien |
| Quentin Debroize | Développeur / Électronicien |
| Marius Charpentier | Chef de projet / Développeur |

> 📅 Projet réalisé de **novembre 2025** à **juin 2026** — 1h/semaine  
> 🏫 [ Nom du Lycée ]

---

## 📦 Contenu du repo

```
maquette-pedagogique-ciel/
│
├── jauge/
│   ├── schema/
│   │   └── Schematic_jauge.png
│   └── programme/
│       └── jauge.ino
│
├── bouton-reflexe/
│   ├── schema/
│   │   └── Schematic_reflex.png
│   └── programme/
│       └── bouton_reflexe.ino
│
├── pong/
│   └── schema/
│       └── Schematic_pong.png
│
├── rfid/
│   └── schema/
│       └── Schematic_rfid.png
│
└── docs/
    ├── Dossier_Projet_Maquette_Pedagogique_CIEL.docx
    └── Annexe_Pedagogique_Maquette_CIEL.docx
```

---

## ⚡ Les 4 circuits

### 🌡️ Circuit 1 — Jauge
> Mesure de distance par ultrason → affichage barre de niveau sur LCD

**Matériel**
- Arduino Uno R3
- Capteur ultrason **HC-SR04**
- Écran **LCD 16×2 I2C**

**Principe**  
Le capteur émet une impulsion ultrasonique et mesure le temps de retour de l'écho. La distance est calculée avec la formule :

```
distance (cm) = durée_echo (µs) × 0.0343 / 2
```

La distance est ensuite convertie en un nombre de segments affichés sur le LCD (plage : **2 cm → 24 cm**).

**Statut** : ✅ Réalisé et programmé

---

### ⚡ Circuit 2 — Bouton Réflexe
> Jeu de mesure du temps de réaction avec anti-rebond et délai aléatoire

**Matériel**
- Arduino / CH340S
- Bouton poussoir (D7, `INPUT_PULLUP`)
- LED rouge + résistance 100 Ω (D10)
- Buzzer 4000 Hz (D2)
- Écran **LCD 16×2 I2C**

**Principe**  
Une LED s'allume après un délai aléatoire (`random(1000, 7001)` ms). Le joueur appuie le plus vite possible. Le temps de réaction est mesuré via `millis()` et affiché en secondes.

```cpp
tempsReactionMs  = millis() - tempsDepart;
tempsReactionSec = tempsReactionMs / 1000.0;
```

L'anti-rebond logiciel (`DEBOUNCE_MS = 50`) garantit la fiabilité des lectures.

**Statut** : ✅ Réalisé et programmé

---

### 🎮 Circuit 3 — Pong
> Jeu rétro contrôlé par capteurs ultrason sans contact

**Matériel**
- Arduino Uno R3
- 2× capteur ultrason **HC-SR04** (un par joueur)
- Afficheur 7 segments **MAX7219** (score, SPI)
- Écran **LCD 16×2 I2C**
- Bouton poussoir (start/reset)

**Principe**  
Chaque joueur contrôle sa raquette en bougeant la main devant son capteur HC-SR04. La distance mesurée est convertie en position de raquette sur l'afficheur.

**Statut** : 📐 Schéma réalisé — programme non finalisé

---

### 🔑 Circuit 4 — Serrure RFID
> Contrôle d'accès par badge sans contact

**Matériel**
- Arduino Uno R3
- Module **RFID RC522** (SPI)
- Relais **5V** (SRD-05VDC)
- Serrure électrique 5V
- LED verte + LED rouge (résistances 180 Ω)

**Principe**  
Le module RC522 lit l'UID d'un badge RFID via le protocole SPI. Si l'UID est dans la liste autorisée, le relais s'active et ouvre la serrure. Sinon, la LED rouge s'allume.

```
Badge autorisé  → LED verte + relais activé → serrure ouverte
Badge inconnu   → LED rouge
```

**Statut** : 📐 Schéma réalisé — programme non finalisé

---

## 🛠️ Installation & utilisation

### Prérequis
- [Arduino IDE](https://www.arduino.cc/en/software) (version 1.8+ ou 2.x)
- Bibliothèques à installer via le gestionnaire de bibliothèques Arduino :

| Bibliothèque | Circuit |
|---|---|
| `LiquidCrystal_I2C` | Jauge, Bouton Réflexe |
| `Wire` | Jauge, Bouton Réflexe |
| `MFRC522` | Serrure RFID |
| `LedControl` | Pong |

### Téléverser un programme

1. Ouvrir le fichier `.ino` du circuit voulu dans l'Arduino IDE
2. Sélectionner la carte : **Arduino Uno**
3. Sélectionner le port COM correspondant
4. Cliquer sur **Téléverser** ▶️

---

## 📐 Schémas électriques

Tous les schémas ont été réalisés avec [EasyEDA](https://easyeda.com).

| Circuit | Schéma |
|---------|--------|
| Jauge | [`jauge/schema/`](./jauge/schema/) |
| Bouton Réflexe | [`bouton-reflexe/schema/`](./bouton-reflexe/schema/) |
| Pong | [`pong/schema/`](./pong/schema/) |
| Serrure RFID | [`rfid/schema/`](./rfid/schema/) |

---

## 📚 Documentation

Le dossier complet du projet (cahier des charges, planning, explications mathématiques, description des programmes par blocs) est disponible dans le dossier [`docs/`](./docs/).

---

## 🎓 Contexte pédagogique

Ce projet a été réalisé dans le cadre du **Bac Pro CIEL** pour présenter la filière lors des journées portes ouvertes du lycée. Il illustre les compétences suivantes :

- Conception de schémas électroniques
- Programmation embarquée en **C++** sur Arduino
- Utilisation de protocoles de communication (**I2C**, **SPI**)
- Gestion du temps réel et des entrées/sorties
- Rédaction d'une documentation technique complète

---

## 📄 Licence

Projet réalisé à des fins pédagogiques — [ Nom du Lycée ] — 2025/2026
