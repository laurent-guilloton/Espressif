# XIAO ESP32-C6 - Capteur SHT31 + Ruban LED WS2812B

Projet Arduino pour la carte Seeed Studio XIAO ESP32-C6 intégrant :
- Capteur de température et humidité SHT31 (Grove)
- Ruban LED WS2812B (26 LEDs) **contrôlable via Home Assistant**
- Communication Zigbee pour domotique

## Matériel requis

### Composants
- Seeed Studio XIAO ESP32-C6
- Capteur Grove SHT31 (température + humidité)
- Ruban LED WS2812B (26 LEDs)
- Câbles Grove et connecteurs
- Alimentation 5V (pour le ruban LED)

### Connexions
```
XIAO ESP32-C6    →    SHT31 Grove
D2 (SDA)         →    SDA
D3 (SCL)         →    SCL
3V3              →    VCC
GND              →    GND

XIAO ESP32-C6    →    WS2812B
D1               →    DIN
5V               →    VCC
GND              →    GND
```

## Bibliothèques requises

Installer via le gestionnaire de bibliothèques Arduino :

1. **Adafruit SHT31 Library**
   - Recherche: "Adafruit SHT31"
   - Version: 2.2.0 ou supérieure

2. **Adafruit NeoPixel Library**
   - Recherche: "Adafruit NeoPixel"
   - Version: 1.11.0 ou supérieure

3. **Zigbee Libraries** (spécifiques ESP32-C6)
   - Recherche: "Zigbee" et "ZigbeeHA"
   - Compatible ESP32-C6

## Installation

1. Cloner ou télécharger ce projet
2. Ouvrir `xiao_esp32c6_sht31_ws2812b.ino` dans l'IDE Arduino
3. Installer les bibliothèques requises
4. Configurer la carte XIAO ESP32-C6 dans l'IDE
5. Téléverser le code

## Configuration

### Configuration Zigbee
Modifier `zigbee_config.h` :
```cpp
#define ZIGBEE_PAN_ID 0x1234        // ID de votre réseau
#define ZIGBEE_CHANNEL 11           // Canal (11-26)
#define ZIGBEE_INTERVAL 30000       // Fréquence d'envoi (ms)
```

### Configuration matérielle
Modifier les constantes dans le fichier `.ino` :
```cpp
#define LED_PIN D1           // Pin pour les LEDs
#define LED_COUNT 26         // Nombre de LEDs
#define SHT31_ADDR 0x44      // Adresse I2C du capteur
```

## Fonctionnalités

### Capteur SHT31
- Mesure température : -40°C à +125°C (±0.3°C)
- Mesure humidité : 0% à 100% (±2%)
- Communication I2C
- Auto-calibrage

### Ruban LED WS2812B avec contrôle Home Assistant
- 26 LEDs RGB individuellement contrôlables
- **Contrôle complet via Home Assistant**
- Indication visuelle de la température (mode automatique)
- Animation de démarrage
- Effet de dégradé basé sur la température

### Modes LED contrôlables via Home Assistant

#### Mode Température (MODE_TEMPERATURE)
- Indication automatique par couleur selon la température
- **Bleu** : < 0°C (Très froid)
- **Cyan** : 0-10°C (Froid)
- **Vert** : 10-20°C (Frais)
- **Jaune** : 20-25°C (Confortable)
- **Orange** : 25-30°C (Chaud)
- **Rouge** : > 30°C (Très chaud)

#### Mode Couleur Unie (MODE_SOLID)
- Couleur fixe choisie dans Home Assistant
- Luminosité ajustable (1-100%)

#### Mode Arc-en-ciel (MODE_RAINBOW)
- Animation arc-en-ciel fluide
- Activé automatiquement si luminosité > 80%

#### Mode Respiration (MODE_BREATHING)
- Effet de respiration avec la couleur choisie
- Activé automatiquement si luminosité < 30%

#### Mode Poursuite (MODE_CHASE)
- Animation de poursuite sur le ruban LED
- 3 LEDs allumées en déplacement

#### Mode Arrêt (MODE_OFF)
- Toutes les LEDs éteintes

## Intégration Home Assistant

### Configuration Zigbee2MQTT

Le dispositif crée automatiquement 3 entités dans Home Assistant :

1. **Capteur de température** : `sensor.xiao_temp_XXXX`
2. **Capteur d'humidité** : `sensor.xiao_hum_XXXX`
3. **Ruban LED** : `light.xiao_led_XXXX`

### Exemple de configuration YAML

```yaml
# configuration.yaml
light:
  - platform: mqtt
    name: "XIAO LED Strip"
    state_topic: "zigbee2mqtt/XIAO_LED_XXXX"
    command_topic: "zigbee2mqtt/XIAO_LED_XXXX/set"
    brightness: true
    rgb: true
    effect: true
    effect_list:
      - "temperature"
      - "solid"
      - "rainbow"
      - "breathing"
      - "chase"
      - "off"

sensor:
  - platform: mqtt
    name: "XIAO Temperature"
    state_topic: "zigbee2mqtt/XIAO_TEMP_XXXX"
    unit_of_measurement: "°C"
    device_class: temperature
    
  - platform: mqtt
    name: "XIAO Humidity"
    state_topic: "zigbee2mqtt/XIAO_HUM_XXXX"
    unit_of_measurement: "%"
    device_class: humidity
```

### Automatisations exemples

#### 1. Contrôle automatique selon la température
```yaml
automation:
  - alias: "LED Temperature Control"
    trigger:
      - platform: state
        entity_id: sensor.xiao_temperature
    action:
      - service: light.turn_on
        entity_id: light.xiao_led_strip
        data_template:
          effect: "temperature"
```

#### 2. Notification visuelle
```yaml
automation:
  - alias: "High Temperature Alert"
    trigger:
      - platform: numeric_state
        entity_id: sensor.xiao_temperature
        above: 30
    action:
      - service: light.turn_on
        entity_id: light.xiao_led_strip
        data:
          rgb_color: [255, 0, 0]
          brightness: 100
          effect: "breathing"
```

#### 3. Mode ambiance soirée
```yaml
automation:
  - alias: "Evening Ambiance"
    trigger:
      - platform: time
        at: "20:00:00"
    action:
      - service: light.turn_on
        entity_id: light.xiao_led_strip
        data:
          rgb_color: [255, 100, 50]
          brightness: 40
          effect: "breathing"
```

## États du système

Le projet utilise une machine à états :

1. **STATE_INIT** : Initialisation des composants
2. **STATE_READ_SENSOR** : Lecture du capteur SHT31
3. **STATE_SEND_ZIGBEE** : Envoi des données via Zigbee
4. **STATE_UPDATE_LEDS** : Mise à jour des LEDs
5. **STATE_PROCESS_LED_COMMANDS** : Traitement commandes Home Assistant
6. **STATE_ERROR** : Gestion des erreurs
7. **STATE_SLEEP** : Pause entre les mesures

## Sécurité et optimisation

### Watchdog
- Timeout de 30 secondes pour éviter les blocages
- Redémarrage automatique en cas de problème

### Gestion d'erreurs
- Maximum 3 tentatives par opération
- Compteur d'erreurs avec redémarrage après 10 erreurs
- LEDs rouges clignotantes en cas d'erreur

### Optimisation mémoire
- Structures compactes pour les données
- Variables optimisées
- Mode asynchrone pour les capteurs

## Dépannage

### Problèmes courants
1. **Capteur non détecté**
   - Vérifier les connexions I2C
   - Vérifier l'adresse (0x44 ou 0x45)

2. **LEDs ne s'allument pas**
   - Vérifier l'alimentation 5V
   - Vérifier la connexion D1
   - Vérifier le sens du ruban LED

3. **Home Assistant ne détecte pas le ruban LED**
   - Vérifier la configuration Zigbee2MQTT
   - Redémarrer le coordinateur Zigbee
   - Vérifier les endpoints dans la configuration

4. **Commandes Home Assistant ne fonctionnent pas**
   - Vérifier la connexion Zigbee
   - Consulter les logs dans Serial Monitor
   - Redémarrer le dispositif

### Messages d'erreur
- `ERREUR: Aucun capteur SHT31 détecté` → Problème I2C
- `ERREUR: Impossible de démarrer Zigbee` → Configuration réseau
- `ERREUR: Lecture invalide du capteur` → Capteur défectueux
- `Mode LED changé: X` → Changement de mode via Home Assistant

## Personnalisation

### Ajouter d'autres capteurs
1. Inclure les bibliothèques nécessaires
2. Ajouter les variables globales
3. Implémenter les fonctions de lecture
4. Intégrer dans la machine à états

### Modifier les animations LED
1. Modifier la fonction `updateLEDs()`
2. Ajouter de nouveaux modes dans l'enum `LEDMode`
3. Implémenter les fonctions d'animation
4. Personnaliser les couleurs dans `tempColors[]`

### Ajouter des effets personnalisés
```cpp
// Exemple : Ajouter un effet "Stroboscope"
void updateStrobeMode() {
  static bool state = false;
  if (millis() - ledAnimationTimer > 100) {
    state = !state;
    setLEDColor(state ? currentLEDColor : 0);
    ledAnimationTimer = millis();
  }
}
```

## Licence

Projet open-source - libre à des fins éducatives et personnelles.

## Support

Pour toute question ou problème :
1. Vérifier la section dépannage
2. Consulter la documentation des bibliothèques
3. Vérifier les connexions matérielles
4. Consulter les logs dans le Serial Monitor
