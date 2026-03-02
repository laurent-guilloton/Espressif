# Xiao ESP32-C6 avec Capteur DS18B20 et Communication Zigbee

Ce projet permet de mesurer la température ambiante avec un capteur DS18B20 connecté à une carte Seeed Studio Xiao ESP32-C6 et de transmettre les données via le protocole Zigbee.

## Matériel Requis

- Seeed Studio Xiao ESP32-C6 (supporte nativement Zigbee)
- Capteur de température étanche DS18B20
- Résistance de 4.7kΩ (pull-up)
- Fils de connexion
- Breadboard (optionnel)
- Coordinateur Zigbee ou passerelle Zigbee pour recevoir les données

## Câblage

### Connexion du DS18B20 au Xiao ESP32-C6:

| DS18B20 | Xiao ESP32-C6 | Description |
|---------|---------------|-------------|
| VCC (Rouge) | 3V3 | Alimentation 3.3V |
| GND (Noir) | GND | Masse |
| DQ (Jaune) | D2 | Ligne de données |

**Important:** Connecter une résistance de 4.7kΩ entre la ligne de données (D2) et VCC (pull-up).

## Configuration

### 1. Installation des bibliothèques Arduino

**Bibliothèques requises:**
- OneWire par Jim Studt
- DallasTemperature par Miles Burton
- Zigbee (bibliothèque communautaire pour ESP32)
- ZigbeeHA (extension Home Automation)

### 2. Configuration de la carte

1. Ouvrir l'IDE Arduino
2. Sélectionner "Seeed XIAO ESP32-C6" dans Outils > Carte
3. Choisir le port COM approprié

### 3. Configuration Zigbee

Le fichier `zigbee_config.h` contient les paramètres Zigbee personnalisables:
- **PAN ID:** Identifiant du réseau (0x1234 par défaut)
- **Canal:** Canal de communication (11 par défaut)
- **Clés de sécurité:** À personnaliser pour la production

La bibliothèque Zigbee.h gère automatiquement la configuration du coordinateur.

### 4. Téléversement

1. Ouvrir le fichier `xiao_esp32c6_ds18b20.ino`
2. Téléverser le code sur la carte

## Fonctionnalités

### Capteur de température
- Mesure en Celsius et Fahrenheit
- Résolution 12 bits pour plus de précision
- Détection automatique des capteurs
- Gestion des erreurs de connexion

### Communication Zigbee
- Mode coordinateur Zigbee
- Device: Capteur de température Home Automation
- Cluster: Temperature Measurement (ZCL)
- Envoi automatique toutes les 10 secondes
- Format standard Zigbee HA (Home Automation)

## Sortie Série

Exemple de sortie dans le moniteur série:

```
=== Xiao ESP32-C6 - Capteur DS18B20 avec Zigbee ===
Initialisation du capteur de température...
Nombre de capteurs DS18B20 détectés: 1
Capteur initialisé avec succès!
Initialisation de Zigbee...
Zigbee initialisé avec succès!
Début des mesures de température avec communication Zigbee...
----------------------------------------
Température: 23.45 °C / 74.21 °F
Température envoyée via Zigbee: 23.45 °C
Température: 23.67 °C / 74.61 °F
Température envoyée via Zigbee: 23.67 °C
```

## Réception des données Zigbee

### Avec Home Assistant
1. Installer Zigbee2MQTT ou intégration Zigbee native
2. Ajouter le coordinateur Zigbee (ConBee II, Sonoff Zigbee Bridge, etc.)
3. Le capteur sera automatiquement découvert comme "Temperature Sensor"

### Avec Zigbee2MQTT
Le capteur apparaîtra avec les attributs suivants:
```json
{
  "temperature": 23.45,
  "unit": "°C",
  "device": "XIAO_ESP32C6_Temp_Sensor"
}
```

### Avec d'autres passerelles Zigbee
- Philips Hue Bridge (via第三方固件)
- Tuya Zigbee Gateway
- Aqara Gateway

## Personnalisation

### Paramètres modifiables
- **Pin de données:** Modifier `ONE_WIRE_BUS` dans le code
- **Intervalle de mesure:** Modifier `TEMP_MEASUREMENT_INTERVAL_MS` dans `zigbee_config.h`
- **Canal Zigbee:** Modifier `ZIGBEE_CHANNEL` (11-26)
- **Résolution du capteur:** Changer `sensors.setResolution(bits)` (9-12 bits)

### Sécurité
Pour la production, personnaliser:
- PAN ID unique
- Clés réseau dans `zigbee_config.h`
- Activer le mode sécurisé avec Install Code

## Dépannage

### Problèmes courants
- **Capteur non détecté:** Vérifier les connexions et la résistance pull-up
- **Zigbee ne démarre pas:** Vérifier que l'ESP32-C6 est bien configuré pour Zigbee
- **Pas de communication Zigbee:** Vérifier le canal et la configuration du coordinateur
- **Lecture incorrecte:** Assurer une bonne alimentation 3.3V

### Messages d'erreur
- `Zigbee non initialisé`: Problème de configuration Zigbee
- `DEVICE_DISCONNECTED_C`: Capteur DS18B20 déconnecté
- `ERREUR: Capteur DS18B20 défectueux`: Problème de communication avec le capteur

## Notes importantes

- Le Xiao ESP32-C6 supporte nativement Zigbee, pas besoin de module externe
- L'antenne Zigbee est intégrée à la carte
- La consommation augmente avec Zigbee activé (~15mA supplémentaires)
- La portée Zigbee est d'environ 10-20m en intérieur
