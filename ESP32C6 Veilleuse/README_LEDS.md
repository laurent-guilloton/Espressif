# Ruban LED WS2812B avec Xiao ESP32-C6

Ce document décrit l'intégration d'un ruban LED WS2812B de 26 LEDs contrôlé via Home Assistant et Zigbee.

## Matériel Requis

- **Xiao ESP32-C6** - Carte de développement avec support Zigbee
- **Ruban LED WS2812B** - 26 LEDs (ou autre nombre)
- **Capteur SHT31** - Pour les mesures de température/humidité
- **Fils de connexion** - Pour relier les composants

## Connexions Matérielles

| Composant | Xiao ESP32-C6 | Description |
|-----------|----------------|-------------|
| Ruban LED WS2812B | D2 (GPIO2) | Data In du ruban LED |
| SHT31 SDA | GPIO5 | I2C Data |
| SHT31 SCL | GPIO6 | I2C Clock |
| SHT31 VCC | 3.3V | Alimentation capteur |
| SHT31 GND | GND | Masse commune |
| Ruban LED VCC | 5V | Alimentation ruban (externe recommandée) |
| Ruban LED GND | GND | Masse commune |

⚠️ **Important**: Le ruban LED WS2812B nécessite une alimentation 5V séparée. N'alimentez pas le ruban directement depuis le 3.3V de l'ESP32.

## Modes LED Disponibles

### 1. **LED_OFF** (0)
- Toutes les LEDs éteintes
- Mode par défaut pour économie d'énergie

### 2. **LED_SOLID** (1)
- Couleur fixe sur toutes les LEDs
- Paramètres: RGB (0-255) + luminosité

### 3. **LED_RAINBOW** (2)
- Animation arc-en-ciel fluide
- Toutes les LEDs avec dégradé de couleurs

### 4. **LED_TEMPERATURE** (3)
- Couleur basée sur la température actuelle
- Bleu (<15°C) → Cyan (15-20°C) → Vert (20-25°C) → Jaune (25-30°C) → Rouge (>30°C)

### 5. **LED_HUMIDITY** (4)
- Couleur basée sur l'humidité actuelle
- Jaune (<30%) → Vert (30-50%) → Cyan (50-70%) → Bleu (>70%)

### 6. **LED_BREATHING** (5)
- Effet de respiration avec la couleur choisie
- Variation douce de luminosité

### 7. **LED_FLASH** (6)
- Clignotement on/off toutes les 500ms
- Avec la couleur choisie

### 8. **LED_CHASE** (7)
- Effet de poursuite sur le ruban
- 5 LEDs allumées qui se déplacent

## Configuration Zigbee

Le ruban LED est contrôlé via le cluster Zigbee `COLOR_CONTROL`:

### Attributs disponibles:
- `current_hue` (uint16) - Teinte (0-360°)
- `current_saturation` (uint8) - Saturation (0-255)
- `current_x` (uint16) - Coordonnée chromatique X
- `current_y` (uint16) - Coordonnée chromatique Y
- `on_off` (boolean) - Marche/arrêt
- `current_level` (uint8) - Luminosité (0-255)

## Contrôle via Home Assistant

### Méthode 1: MQTT (recommandée)

```yaml
# Dans configuration.yaml
light:
  - platform: mqtt
    name: "Ruban LED Salon"
    state_topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/light"
    command_topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/light/set"
    rgb_command_topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/light/rgb/set"
    brightness_command_topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/light/brightness/set"
```

### Méthode 2: Sélecteur de mode

```yaml
input_select:
  led_mode_salon:
    name: "Mode Ruban LED Salon"
    options:
      - "Éteint"
      - "Couleur fixe"
      - "Arc-en-ciel"
      - "Température"
      - "Humidité"
      - "Respiration"
      - "Clignotement"
      - "Poursuite"
```

### Méthode 3: Automatisations

```yaml
automation:
  # Mode automatique selon température
  - alias: "LED selon température"
    trigger:
      - platform: numeric_state
        entity_id: sensor.temperature_salon
        above: 25
    action:
      - service: mqtt.publish
        data:
          topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/led_mode/set"
          payload: "3"  # Mode température
```

## Messages MQTT

### Changement de mode LED:
```
Topic: zigbee2mqtt/Capteur SHT31 avec LEDs/led_mode/set
Payload: "0" à "7"
```

### Changement de couleur RGB:
```
Topic: zigbee2mqtt/Capteur SHT31 avec LEDs/light/rgb/set
Payload: {"r": 255, "g": 0, "b": 128}
```

### Changement de luminosité:
```
Topic: zigbee2mqtt/Capteur SHT31 avec LEDs/light/brightness/set
Payload: "150"  # 0-255
```

## Performance et Consommation

### Consommation selon mode:
- **LED_OFF**: ~0mA
- **LED_SOLID**: ~15mA par LED (390mA total @ 50%)
- **LED_RAINBOW**: ~20mA par LED (520mA total)
- **LED_TEMPERATURE/HUMIDITY**: ~15mA par LED
- **LED_BREATHING**: ~10-20mA par LED (variable)
- **LED_FLASH**: ~15mA par LED (moyenne)
- **LED_CHASE**: ~10-15mA par LED (moyenne)

### Optimisations:
- Mise à jour toutes les 50ms maximum
- Mode OFF par défaut en deep sleep
- Luminosité configurable pour économie d'énergie

## Dépannage

### Problèmes courants:

**Ruban ne s'allume pas:**
- Vérifiez l'alimentation 5V externe
- Confirmez la connexion sur GPIO2
- Testez avec un simple sketch Arduino

**Couleurs incorrectes:**
- Vérifiez l'ordre des fils (Data, Clock, etc.)
- Confirmez le type WS2812B vs WS2812
- Ajustez la configuration NEO_GRB/NEO_RGB

**Flickering ou instabilité:**
- Ajoutez un condensateur 1000µF sur l'alimentation
- Utilisez une résistance 220-470Ω sur la ligne Data
- Vérifiez la longueur maximale des câbles (<5m recommandé)

**Communication Zigbee défaillante:**
- Éloignez le ruban LED du module Zigbee
- Utilisez des câbles blindés
- Vérifiez l'alimentation séparée

## Personnalisation

### Modification du nombre de LEDs:
```cpp
#define LED_COUNT 26  // Changez selon votre ruban
```

### Changement de la broche:
```cpp
#define LED_PIN 2    // Changez si nécessaire
```

### Ajout de nouveaux modes:
1. Ajoutez une valeur à l'enum `LedMode`
2. Implémentez le mode dans `updateLeds()`
3. Ajoutez le support dans Home Assistant

## Sécurité

⚠️ **Précautions importantes:**
- Utilisez une alimentation 5V de qualité suffisante
- Ne dépassez pas 3A pour 26 LEDs @ pleine puissance
- Ajoutez une protection contre les surtensions
- Maintenez une bonne ventilation
- Évitez les températures extrêmes (>60°C)

## Maintenance

### Nettoyage:
- Dépoussiérage régulier du ruban
- Vérification des connexions électriques
- Inspection visuelle des LEDs défectueuses

### Mises à jour:
- Mettez à jour le firmware ESP32 régulièrement
- Vérifiez la compatibilité des bibliothèques
- Testez après chaque mise à jour

## Exemples d'Utilisation

### Veilleuse intelligente:
```yaml
# Mode couleur chaude le soir
automation:
  - alias: "Mode soirée"
    trigger:
      - platform: time
        at: "20:00:00"
    action:
      - service: light.turn_on
        target:
          entity_id: light.ruban_led_salon
        data:
          rgb_color: [255, 100, 50]
          brightness: 30
      - service: mqtt.publish
        data:
          topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/led_mode/set"
          payload: "1"  # Mode fixe
```

### Indicateur météo:
```yaml
# Mode température automatique
automation:
  - alias: "Affichage température"
    trigger:
      - platform: state
        entity_id: sensor.temperature_salon
    action:
      - service: mqtt.publish
        data:
          topic: "zigbee2mqtt/Capteur SHT31 avec LEDs/led_mode/set"
          payload: "3"  # Mode température
```

Cette intégration offre une solution complète de veilleuse intelligente avec feedback visuel basé sur les conditions environnementales réelles.
