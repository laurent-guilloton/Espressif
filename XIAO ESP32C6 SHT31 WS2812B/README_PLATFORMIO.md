# XIAO ESP32-C6 SHT31 + WS2812B avec PlatformIO

## Configuration Zigbee

### Problème résolu
Les bibliothèques `<Zigbee.h>` et `<ZigbeeHA.h>` n'existent pas dans l'écosystème Arduino/PlatformIO standard.

### Solution implémentée
- **Wrapper Zigbee** (`zigbee_wrapper.h`) : Interface compatible avec le code existant
- **Configuration PlatformIO** (`platformio.ini`) : Support ESP32-C6 avec Zigbee
- **Partition Zigbee** (`zigbee.csv`) : Espace dédié pour le stockage Zigbee

## Installation PlatformIO

1. **Installer l'extension PlatformIO** dans VS Code
2. **Ouvrir le projet** dans le dossier actuel
3. **Compiler** : `pio run`
4. **Uploader** : `pio run --target upload`
5. **Monitor** : `pio device monitor`

## Configuration matérielle

- **Board** : Seeed XIAO ESP32-C6
- **Capteur** : SHT31 (I2C adresse 0x44)
- **LEDs** : WS2812B (26 LEDs, pin D1)
- **I2C** : SDA=D2, SCL=D3

## Étapes suivantes

1. **Tester la compilation** avec le wrapper actuel (simulation Zigbee)
2. **Implémenter Zigbee réel** avec ESP-IDF si nécessaire
3. **Configurer Home Assistant** pour la découverte automatique

## Notes importantes

- Le wrapper actuel simule Zigbee avec des messages Serial
- Pour une implémentation Zigbee complète, utiliser ESP-IDF Zigbee Stack
- Le code SHT31 et WS2812B fonctionne déjà correctement

## Dépendances

- `Adafruit SHT31 Library@^2.2.0`
- `Adafruit NeoPixel@^1.12.0`
- ESP32 Arduino Core v3.0+
- PlatformIO ESP32 platform v6.6.0+
