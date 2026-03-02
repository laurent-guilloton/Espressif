# Capteur SHT31 avec Xiao ESP32-C6 pour Home Assistant via Zigbee

Ce projet permet de mesurer la température et l'humidité avec un capteur SHT31 et d'envoyer les données à Home Assistant via le protocole Zigbee en utilisant une carte Xiao ESP32-C6.

## Matériel requis

- **Xiao ESP32-C6** - Carte de développement avec support Zigbee intégré
- **Capteur SHT31** - Capteur de température et d'humidité I2C
- **Fils de connexion** - Pour relier le capteur à la carte

## Connexions

| Capteur SHT31 | Xiao ESP32-C6 |
|---------------|---------------|
| VIN/VCC       | 3.3V          |
| GND           | GND           |
| SDA           | GPIO5         |
| SCL           | GPIO6         |

## Installation

### 1. Préparation de l'environnement Arduino

1. Installez l'IDE Arduino
2. Ajoutez le support pour les cartes ESP32:
   - Fichier → Préférences → URL de gestionnaire de cartes supplémentaires:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json
   ```
3. Outils → Gestionnaire de cartes → Cherchez "ESP32" et installez "ESP32 by Espressif Systems"

### 2. Bibliothèques requises

Installez les bibliothèques suivantes via le Gestionnaire de bibliothèques:
- `Adafruit SHT31 Library`
- `Wire` (inclus dans l'IDE Arduino)

### 3. Configuration du code

1. Ouvrez le fichier `xiao_esp32c6_sht31_zigbee.ino`
2. Vérifiez les broches I2C (GPIO5 pour SDA, GPIO6 pour SCL par défaut)
3. Téléversez le code sur votre Xiao ESP32-C6

## Configuration Home Assistant

### Option 1: Zigbee intégré (recommandé)

1. Assurez-vous que Zigbee est activé dans Home Assistant
2. Mettez votre appareil en mode appairage (redémarrage)
3. Dans Home Assistant: Paramètres → Appareils et services → Zigbee → Ajouter un appareil
4. Suivez les instructions pour appairer votre capteur

### Option 2: Zigbee2MQTT

1. Installez Zigbee2MQTT
2. Configurez votre coordonnateur Zigbee
3. Ajoutez la configuration à votre `configuration.yaml` (voir fichier `configuration_home_assistant.yaml`)
4. Redémarrez Home Assistant

## Fonctionnalités

- **Mesure de température**: Précision de ±0.3°C
- **Mesure d'humidité**: Précision de ±2%
- **Communication Zigbee**: Faible consommation d'énergie
- **Intervalle de mesure**: 30 secondes (configurable)
- **Gestion d'erreurs**: Détection des pannes de capteur

## Dépannage

### Le capteur n'est pas détecté
- Vérifiez les connexions I2C
- Assurez-vous que le capteur est alimenté en 3.3V
- Vérifiez les adresses I2C (0x44 par défaut)

### Pas de communication Zigbee
- Vérifiez que le canal Zigbee correspond (canal 11 par défaut)
- Assurez-vous que Home Assistant est configuré pour recevoir des appareils
- Redémarrez l'appareil et Home Assistant

### Données incorrectes
- Vérifiez l'étalonnage du capteur SHT31
- Assurez-vous que le capteur n'est pas exposé à des conditions extrêmes

## Personnalisation

### Modifier l'intervalle de mesure
Changez la constante `READ_INTERVAL` dans le code:
```cpp
const unsigned long READ_INTERVAL = 60000; // 60 secondes
```

### Modifier les broches I2C
Changez les broches dans la fonction `initSHT31()`:
```cpp
Wire.begin(SDA_PIN, SCL_PIN); // Remplacez par vos broches
```

### Ajouter d'autres capteurs
Vous pouvez facilement ajouter d'autres capteurs I2C en suivant le même pattern.

## Licence

Ce projet est sous licence MIT.
