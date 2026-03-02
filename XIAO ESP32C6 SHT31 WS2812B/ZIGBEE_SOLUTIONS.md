# Solutions Zigbee pour ESP32-C6

## 🥇 Solution 1 : PlatformIO + ESP-IDF (Recommandé)

**Avantages :**
- Support Zigbee natif ESP-IDF
- Contrôle total du stack Zigbee
- Compatible avec ton code existant

**Configuration :**
- Framework : `espidf` (pas arduino)
- Build flags : `-DCONFIG_ZB_ENABLED=1`
- Partition : `zigbee.csv`

**Limitations :**
- Code plus complexe (ESP-IDF pur)
- Bibliothèques Arduino à adapter

## 🥈 Solution 2 : Arduino IDE + ESP32 Core

**Avantages :**
- Interface simple
- Bibliothèques Arduino compatibles
- Moins de configuration

**Installation :**
1. Installer **Arduino IDE 2.0+**
2. Ajouter URL ESP32 dans préférences :
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Installer **ESP32 Boards** (v3.0+)
4. Sélectionner **XIAO ESP32-C6**

**Limitations :**
- Support Zigbee limité
- Moins flexible que ESP-IDF

## 🥉 Solution 3 : VS Code + Arduino CLI

**Approche hybride :**
- VS Code comme éditeur
- Arduino CLI pour compilation/upload
- Extension Arduino officielle

## 🎯 Recommandation pour ton projet

**Utilise Solution 1 (PlatformIO + ESP-IDF) :**
- Le XIAO ESP32-C6 a le meilleur support Zigbee avec ESP-IDF
- Tu peux garder ton code existant avec le wrapper
- Évolutif pour des fonctionnalités Zigbee avancées

## Étapes immédiates

1. **Tester PlatformIO ESP-IDF** avec le wrapper
2. **Vérifier la compilation** du SHT31 + WS2812B
3. **Implémenter Zigbee réel** progressivement

Le wrapper `zigbee_espidf.h` permet une transition douce vers ESP-IDF Zigbee.
