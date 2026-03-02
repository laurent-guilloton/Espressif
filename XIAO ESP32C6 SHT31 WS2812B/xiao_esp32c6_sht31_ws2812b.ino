
// Inclusion des bibliothèques nécessaires
#include "zigbee_config.h"           // Configuration personnalisée Zigbee
#include "zigbee_wrapper.h"          // Wrapper Zigbee pour compatibilité
#include <Wire.h>                     // Bibliothèque I2C pour communication SHT31
#include <Adafruit_SHT31.h>           // Bibliothèque pour le capteur de température/humidité SHT31
#include <Adafruit_NeoPixel.h>        // Bibliothèque pour le ruban LED WS2812B

// Définitions des constantes de configuration matérielle
#define SHT31_ADDR 0x44      // Adresse I2C par défaut du capteur SHT31 (0x44 ou 0x45)
#define LED_PIN D1           // Pin numérique connecté au ruban LED WS2812B
#define LED_COUNT 26         // Nombre total de LEDs sur le ruban
#define MAX_RETRY_COUNT 3    // Nombre maximal de tentatives en cas d'échec
#define TEMP_CHANGE_THRESHOLD 0.2f // Seuil de changement de température pour déclencher un envoi (°C)
#define HUM_CHANGE_THRESHOLD 2.0f  // Seuil de changement d'humidité pour déclencher un envoi (%)
#define WATCHDOG_TIMEOUT 30000    // Timeout du watchdog pour éviter les blocages (30 secondes)
#define SLEEP_DURATION 2000       // Durée de veille entre les mesures (2 secondes)
#define COLOR_RANGES 6            // Nombre total de plages de température

// Énumération des états possibles du système (machine à états)
// Cette structure permet de contrôler le déroulement du programme de manière ordonnée
enum SystemState {
  STATE_INIT,                // État d'initialisation des composants
  STATE_READ_SENSOR,         // État de lecture du capteur SHT31
  STATE_SEND_ZIGBEE,         // État d'envoi des données via Zigbee
  STATE_UPDATE_LEDS,         // État de mise à jour des LEDs
  STATE_PROCESS_LED_COMMANDS, // État de traitement des commandes LED reçues
  STATE_ERROR,               // État d'erreur du système
  STATE_SLEEP                // État de veille entre les mesures
};

// Énumération des différents modes d'affichage des LEDs
// Permet de switcher entre différents effets visuels
enum LEDMode {
  MODE_TEMPERATURE,  // Mode où la couleur reflète la température actuelle
  MODE_SOLID,        // Mode couleur fixe
  MODE_RAINBOW,      // Mode arc-en-ciel animé
  MODE_BREATHING,    // Mode respiration (variation de luminosité)
  MODE_CHASE,        // Mode poursuite (LEDs qui se déplacent)
  MODE_OFF           // Mode LEDs éteintes
};

// Instanciation des objets principaux
Adafruit_SHT31 sht31 = Adafruit_SHT31();                    // Objet pour communiquer avec le capteur SHT31
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); // Objet pour contrôler le ruban LED
Zigbee zigbee;                                              // Objet principal de communication Zigbee
ZigbeeHATemperatureSensor tempSensor(ZIGBEE_ENDPOINT);      // Capteur de température virtuel Zigbee HA
ZigbeeHAHumiditySensor humSensor(ZIGBEE_ENDPOINT_HUM);      // Capteur d'humidité virtuel Zigbee HA
ZigbeeHADimmableLight ledController(ZIGBEE_ENDPOINT_LED);   // Contrôleur de lumière LED virtuel Zigbee HA

// Variables globales pour stocker les données du capteur
struct SensorData {
  float temperature;    // Température en degrés Celsius
  float humidity;       // Humidité relative en pourcentage
  bool valid;          // Indicateur de validité des données
  unsigned long timestamp; // Timestamp de la dernière lecture
} currentReading; // Instance contenant la lecture actuelle

// Variables de suivi des états précédents pour éviter les envois redondants
volatile float lastSentTemp = -999.0;  // Dernière température envoyée (valeur initiale impossible)
volatile float lastSentHum = -999.0;   // Dernière humidité envoyée (valeur initiale impossible)

// Variables de gestion de la machine à états
SystemState currentState = STATE_INIT;  // État actuel du système
unsigned long stateTimer = 0;           // Timer pour suivre la durée dans chaque état
uint8_t retryCount = 0;                 // Compteur de tentatives en cas d'échec
uint16_t errorCount = 0;                // Compteur total d'erreurs système
unsigned long sleepStartTime = 0;       // Timer pour la gestion non-bloquante du sommeil

// Variables de statut de connexion des composants
bool sensorConnected = false;   // True si le capteur SHT31 est connecté et fonctionnel
bool zigbeeConnected = false;   // True si le réseau Zigbee est initialisé
bool ledsConnected = false;     // True si le ruban LED est initialisé

// Variables de contrôle du comportement des LEDs
LEDMode currentLEDMode = MODE_TEMPERATURE; // Mode d'affichage LED actuel
uint32_t currentLEDColor = 0x000000;       // Couleur actuelle des LEDs (format RGB 32-bit)
uint8_t currentLEDBrightness = 50;        // Luminosité actuelle (0-100%)

// Variables pour les animations LED
unsigned long ledAnimationTimer = 0;       // Timer pour synchroniser les animations
uint16_t rainbowHue = 0;                   // Teinte actuelle pour le mode arc-en-ciel
uint8_t breathingStep = 0;                 // Étape actuelle pour l'animation de respiration
uint8_t chasePosition = 0;                 // Position actuelle pour l'animation de poursuite

// Structure pour définir les plages de couleurs selon la température
struct ColorRange {
  uint32_t color;      // Couleur au format RGB 32-bit
  float minTemp;       // Température minimale de la plage
  float maxTemp;       // Température maximale de la plage
  const char* description;  // Description textuelle (const char* économise la mémoire vs String)
};

// Tableau des plages de couleurs pour la visualisation de température
// Chaque plage correspond à une sensation thermique spécifique
ColorRange tempColors[] = {
  {0x0000FF, -10.0f, 0.0f, "Froid"},
  {0x00FFFF, 0.0f, 10.0f, "Frais"},
  {0x00FF00, 10.0f, 20.0f, "Cool"},
  {0xFFFF00, 20.0f, 25.0f, "Confort"},
  {0xFF8800, 25.0f, 30.0f, "Chaud"},
  {0xFF0000, 30.0f, 50.0f, "Tres chaud"}
};

// Active/désactive les messages de débogage (0 = désactivé, 1 = activé)
#define DEBUG_MODE 1

// Fonction helper pour afficher les messages de débogage conditionnellement
inline void debugPrint(const char* msg) {
  #if DEBUG_MODE
    Serial.println(msg);
  #endif
}

inline void debugPrintValue(const char* label, float value, const char* unit) {
  #if DEBUG_MODE
    Serial.print(label);
    Serial.print(value);
    Serial.println(unit);
  #endif
}

/**
 * Initialise le capteur de température/humidité SHT31
 * @return true si l'initialisation réussit, false sinon
 */
bool initSensor() {
  // Initialisation I2C avec les pins spécifiques au XIAO ESP32-C6
  Wire.begin(D2, D3);  // SDA = D2, SCL = D3
  
  // Tentative de communication avec le capteur à l'adresse I2C spécifiée
  if (!sht31.begin(SHT31_ADDR)) {
    Serial.println("ERREUR: Impossible de trouver le capteur SHT31");
    return false;
  }

  #if DEBUG_MODE
    Serial.println("Capteur SHT31 detecte avec succes!");
  #endif
  sht31.heater(false); // Désactiver le chauffage interne pour des mesures précises
  
  return true;
}

/**
 * Initialise le ruban LED WS2812B
 * @return true si l'initialisation réussit, false sinon
 */
bool initLEDs() {
  strip.begin();                    // Initialisation de la communication avec le ruban LED
  strip.show();                     // Éteindre toutes les LEDs au démarrage
  strip.setBrightness(50);           // Luminosité initiale à 50%
  
  // Test de démarrage: allume les LEDs une par une en blanc
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
    strip.show();
    delay(10); // Petite pause pour créer un effet de balayage
  }
  
  delay(500); // Pause avec toutes les LEDs allumées
  
  // Extinction progressif des LEDs
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, 0);
    strip.show();
    delay(10);
  }
  
  #if DEBUG_MODE
    Serial.println("Ruban LED WS2812B initialise avec succes!");
  #endif
  return true;
}

/**
 * Initialise le réseau Zigbee et configure les appareils virtuels
 * @return true si l'initialisation réussit, false sinon
 */
bool initZigbeeNetwork() {
  // Configuration du réseau Zigbee
  zigbee.setPanId(ZIGBEE_PAN_ID);     // Définition de l'ID du réseau personnel
  zigbee.setChannel(ZIGBEE_CHANNEL); // Canal de communication Zigbee
  zigbee.setCoordinator(true);        // Ce device agit comme coordinateur du réseau

  // Démarrage de la communication Zigbee
  if (!zigbee.begin()) {
    Serial.println("ERREUR: Impossible de démarrer Zigbee");
    return false;
  }

  // Configuration du capteur de température virtuel pour Home Assistant
  uint64_t chipId = ESP.getEfuseMac();
  String chipIdHex = String((uint32_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
  tempSensor.setDeviceId("XIAO_TEMP_" + chipIdHex);
  tempSensor.setManufacturer("Seeed Studio");
  tempSensor.setModel("XIAO ESP32-C6");
  tempSensor.setVersion("1.0");
  tempSensor.setName("Temperature Sensor");

  // Configuration du capteur d'humidité virtuel pour Home Assistant
  humSensor.setDeviceId("XIAO_HUM_" + chipIdHex);
  humSensor.setManufacturer("Seeed Studio");
  humSensor.setModel("XIAO ESP32-C6");
  humSensor.setVersion("1.0");
  humSensor.setName("Humidity Sensor");

  // Configuration du contrôleur LED virtuel pour Home Assistant
  ledController.setDeviceId("XIAO_LED_" + chipIdHex);
  ledController.setManufacturer("Seeed Studio");
  ledController.setModel("XIAO ESP32-C6");
  ledController.setVersion("1.0");
  ledController.setName("LED Strip Controller");

  // Ajout des appareils au réseau Zigbee
  if (!zigbee.addDevice(&tempSensor) || !zigbee.addDevice(&humSensor) || !zigbee.addDevice(&ledController)) {
    Serial.println("ERREUR: Impossible d'ajouter les capteurs Zigbee");
    return false;
  }

  #if DEBUG_MODE
    Serial.println("Reseau Zigbee initialise avec succes!");
  #endif
  return true;
}

// =============================================================================
// SECTION: FONCTIONS DE LECTURE ET DÉCISION
// =============================================================================

/**
 * Lit les données du capteur SHT31 et effectue une validation
 * @return true si la lecture réussit et les données sont valides, false sinon
 */
bool readSensorData() {
  float temp = sht31.readTemperature();  // Lecture de la température en °C
  float hum = sht31.readHumidity();      // Lecture de l'humidité relative en %

  // Validation des valeurs lues (plages acceptables pour un SHT31)
  if (isnan(temp) || isnan(hum) || temp < -40.0 || temp > 125.0 || hum < 0.0 || hum > 100.0) {
    Serial.println("ERREUR: Lecture invalide du capteur SHT31");
    return false;
  }

  // Stockage des données valides
  currentReading.temperature = temp;
  currentReading.humidity = hum;
  currentReading.valid = true;
  currentReading.timestamp = millis();

  debugPrintValue("Temperature: ", temp, "C");
  debugPrintValue("Humidite: ", hum, "%");

  return true;
}

/**
 * Détermine si les données doivent être envoyées via Zigbee
 * Évite les envois redondants en ne transmettant que les changements significatifs
 * @return true si les données doivent être envoyées, false sinon
 */
bool shouldSendData() {
  // Ne pas envoyer si les données actuelles ne sont pas valides
  if (!currentReading.valid)
    return false;

  // Vérifier si c'est le premier envoi ou si la température a changé significativement
  bool tempChanged = (lastSentTemp < -998.0 || 
                      abs(currentReading.temperature - lastSentTemp) >= TEMP_CHANGE_THRESHOLD);
  
  // Vérifier si l'humidité a changé significativement
  bool humChanged = (lastSentHum < -998.0 || 
                     abs(currentReading.humidity - lastSentHum) >= HUM_CHANGE_THRESHOLD);

  // Envoyer si l'un des deux paramètres a changé de manière significative
  return tempChanged || humChanged;
}

// =============================================================================
// SECTION: FONCTIONS DE CONTRÔLE DES LEDS
// =============================================================================

/**
 * Définit une couleur uniforme pour toutes les LEDs du ruban
 * @param color Couleur au format RGB 32-bit (0xRRGGBB)
 */
void setLEDColor(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);  // Applique la même couleur à chaque LED
  }
  strip.show();  // Met à jour l'affichage du ruban LED
}

/**
 * Ajuste la luminosité globale du ruban LED
 * @param brightness Luminosité (0-255)
 */
void setLEDBrightness(uint8_t brightness) {
  strip.setBrightness(brightness);
  strip.show();
}

/**
 * Met à jour l'affichage en mode arc-en-ciel
 * Version optimisée avec pré-calculs des constantes et synchronisation temporelle
 */
void updateRainbowMode() {
  static unsigned long lastUpdate = 0;  // Timer pour synchronisation temporelle
  static const uint16_t hueStep = 65535 / 360;
  static const uint8_t brightnessFactor = 255 / 100;
  static const uint16_t hueOffset = 360 / LED_COUNT;
  static const uint8_t updateInterval = 50;  // Mise à jour toutes les 50ms
  
  // Synchronisation temporelle pour animation fluide indépendante de la vitesse de loop()
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();
  
  const uint8_t brightnessVal = currentLEDBrightness * brightnessFactor;
  
  for (int i = 0; i < LED_COUNT; i++) {
    uint16_t hue = (rainbowHue + (i * hueOffset)) % 360;
    strip.setPixelColor(i, strip.ColorHSV(hue * hueStep, 255, brightnessVal));
  }
  strip.show();
  rainbowHue = (rainbowHue + 2) % 360;
}

/**
 * Met à jour l'affichage en mode respiration
 * Crée une variation douce de la luminosité
 */
void updateBreathingMode() {
  // Utilise une fonction sinusoïdale pour créer l'effet de respiration
  uint8_t brightness = (sin(breathingStep * 0.05) + 1) * currentLEDBrightness / 2;
  strip.setBrightness(brightness);
  setLEDColor(currentLEDColor);
  breathingStep++; // Avance dans l'animation
}

/**
 * Met à jour l'affichage en mode poursuite (chase)
 * Version optimisée avec moins de calculs par itération
 */
void updateChaseMode() {
  strip.clear();  // Effacer toutes les LEDs en une fois (plus rapide)
  
  // Extrait les composantes RGB une seule fois
  const uint8_t baseR = (currentLEDColor >> 16) & 0xFF;
  const uint8_t baseG = (currentLEDColor >> 8) & 0xFF;
  const uint8_t baseB = currentLEDColor & 0xFF;
  
  // Allume 3 LEDs consécutives avec une luminosité dégressive
  for (int i = 0; i < 3; i++) {
    int pos = (chasePosition + i) % LED_COUNT;
    uint8_t brightness = (currentLEDBrightness * (3 - i)) / 3;
    uint8_t scale = (brightness * 255) / 100;  // Conversion pourcentage -> byte
    
    strip.setPixelColor(pos, strip.Color(
      (baseR * scale) >> 8,  // Division par 256 via shift (plus rapide)
      (baseG * scale) >> 8,
      (baseB * scale) >> 8
    ));
  }
  strip.show();
  chasePosition = (chasePosition + 1) % LED_COUNT;
}

/**
 * Traite les commandes LED reçues via Zigbee depuis Home Assistant
 * Permet de contrôler les LEDs depuis l'interface Home Assistant
 */
void processLEDCommands() {
  // Vérifie si de nouvelles commandes ont été reçues via Zigbee
  if (ledController.isStateChanged()) {
    // Récupération des paramètres depuis Home Assistant
    bool isOn = ledController.isOn();           // État ON/OFF
    uint8_t brightness = ledController.getBrightness(); // Luminosité (0-100%)
    uint32_t color = ledController.getColor(); // Couleur (format RGB)
    
    if (!isOn) {
      // Si le device est éteint, passer en mode OFF
      currentLEDMode = MODE_OFF;
      setLEDColor(0);
    } else {
      // Si le device est allumé, mettre à jour les paramètres
      currentLEDColor = color;
      currentLEDBrightness = brightness;
      
      // Détermination automatique du mode en fonction de la luminosité
      if (brightness == 0) {
        currentLEDMode = MODE_OFF;           // Éteint
      } else if (brightness < 30) {
        currentLEDMode = MODE_BREATHING;     // Faible luminosité = respiration
      } else if (brightness > 80) {
        currentLEDMode = MODE_RAINBOW;       // Forte luminosité = arc-en-ciel
      } else {
        currentLEDMode = MODE_SOLID;         // Luminosité moyenne = couleur fixe
      }
    }
    
    debugPrint("Mode LED change");
  }
}

/**
 * Met à jour l'affichage des LEDs en fonction du mode actuel
 * Fonction principale de gestion des animations LED
 */
void updateLEDs() {
  switch (currentLEDMode) {
    case MODE_TEMPERATURE:
      updateTemperatureLEDs();  // Affichage basé sur la température
      break;
      
    case MODE_SOLID:
      strip.setBrightness(currentLEDBrightness);
      setLEDColor(currentLEDColor);  // Couleur fixe
      break;
      
    case MODE_RAINBOW:
      updateRainbowMode();  // Animation arc-en-ciel
      break;
      
    case MODE_BREATHING:
      updateBreathingMode();  // Animation de respiration
      break;
      
    case MODE_CHASE:
      updateChaseMode();  // Animation de poursuite
      break;
      
    case MODE_OFF:
      setLEDColor(0);  // LEDs éteintes
      break;
  }
}

// =============================================================================
// SECTION: FONCTIONS DE VISUALISATION DE TEMPÉRATURE
// =============================================================================

/**
 * Détermine la couleur correspondant à une température donnée
 * @param temp Température en degrés Celsius
 * @return Couleur au format RGB 32-bit
 */
uint32_t getTemperatureColor(float temp) {
  // Parcourt les plages de température pour trouver la couleur correspondante
  for (int i = 0; i < COLOR_RANGES; i++) {
    if (temp >= tempColors[i].minTemp && temp < tempColors[i].maxTemp) {
      return tempColors[i].color;
    }
  }
  // Température hors plage - retourne rouge pour signaler une valeur extrême
  return 0xFF0000;
}

/**
 * Met à jour l'affichage des LEDs en mode température
 * Affiche la température actuelle avec un effet de dégradé
 */
void updateTemperatureLEDs() {
  if (!currentReading.valid) {
    // Mode erreur: LEDs rouges clignotantes
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, (millis() / 200) % 2 ? 0xFF0000 : 0);
    }
  } else {
    // Mode normal: affichage de la température avec dégradé
    uint32_t color = getTemperatureColor(currentReading.temperature);
    
    // Crée un effet de dégradé basé sur la température
    for (int i = 0; i < LED_COUNT; i++) {
      float intensity = (float)i / LED_COUNT; // Intensité progressive
      uint8_t r = (color >> 16) & 0xFF;        // Extraction composante rouge
      uint8_t g = (color >> 8) & 0xFF;         // Extraction composante verte
      uint8_t b = color & 0xFF;               // Extraction composante bleue
      
      // Applique l'intensité progressive pour créer le dégradé
      strip.setPixelColor(i, strip.Color(
        r * intensity,
        g * intensity,
        b * intensity
      ));
    }
  }
  strip.show();
}

/**
 * Affiche une animation de démarrage
 * Animation visuelle qui indique que le système est prêt
 */
void showStartupAnimation() {
  // Animation de démarrage avec 3 cycles
  for (int cycle = 0; cycle < 3; cycle++) {
    // Allume les LEDs une par une
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
      strip.show();
      delay(20); // Pause pour créer l'effet de balayage
    }
    
    // Éteint les LEDs une par une (ordre inverse)
    for (int i = LED_COUNT - 1; i >= 0; i--) {
      strip.setPixelColor(i, 0);
      strip.show();
      delay(20);
    }
  }
}

// =============================================================================
// SECTION: FONCTIONS ZIGBEE
// =============================================================================

/**
 * Envoie les données des capteurs et l'état des LEDs via Zigbee
 * @return true si l'envoi réussit, false sinon
 */
bool sendZigbeeData() {
  bool tempSuccess = false;
  bool humSuccess = false;
  bool ledSuccess = false;

  // Envoi de la température au capteur virtuel
  tempSensor.setTemperature(currentReading.temperature);
  tempSuccess = tempSensor.report();

  // Envoi de l'humidité au capteur virtuel
  humSensor.setHumidity(currentReading.humidity);
  humSuccess = humSensor.report();

  // Envoi de l'état des LEDs au contrôleur virtuel
  ledController.setOn(currentLEDMode != MODE_OFF);
  ledController.setBrightness(currentLEDBrightness);
  ledController.setColor(currentLEDColor);
  ledSuccess = ledController.report();

  // Vérification du succès de tous les envois
  if (tempSuccess && humSuccess && ledSuccess) {
    // Mise à jour des dernières valeurs envoyées pour éviter les doublons
    lastSentTemp = currentReading.temperature;
    lastSentHum = currentReading.humidity;
    Serial.println("Données envoyées avec succès via Zigbee");
    return true;
  } else {
    Serial.println("ERREUR: Échec envoi Zigbee");
    return false;
  }
}

// =============================================================================
// SECTION: SETUP ET LOOP PRINCIPAUX
// =============================================================================

/**
 * Fonction d'initialisation principale (exécutée une fois au démarrage)
 */
void setup() {
  Serial.begin(115200);  // Initialisation de la communication série pour le débogage
  delay(1000);           // Pause pour stabiliser la communication série

  Serial.println("=== XIAO ESP32-C6 - SHT31 + WS2812B ===");
  Serial.println("Version 2.0 - Capteur température/humidité + LEDs contrôlables via HA");
  
  // Vérification du support Zigbee sur ESP32-C6
  #if CONFIG_ZB_ENABLED
    Serial.println("Support Zigbee ESP32-C6 détecté");
  #else
    Serial.println("AVERTISSEMENT: Zigbee non activé dans le core ESP32");
  #endif
  
  // Initialisation de la machine à états
  currentState = STATE_INIT;
  stateTimer = millis();
}

/**
 * Boucle principale du programme (machine à états)
 * Gère le déroulement séquentiel des opérations
 */
void loop() {
  // Affiche l'état actuel de la machine à états (uniquement en mode debug et sur changement)
  #if DEBUG_MODE
  static SystemState lastReportedState = STATE_INIT;
  if (currentState != lastReportedState) {
    lastReportedState = currentState;
    switch(currentState) {
      case STATE_INIT: debugPrint("Etat: Initialisation"); break;
      case STATE_READ_SENSOR: debugPrint("Etat: Lecture capteur"); break;
      case STATE_SEND_ZIGBEE: debugPrint("Etat: Envoi Zigbee"); break;
      case STATE_UPDATE_LEDS: debugPrint("Etat: MAJ LEDs"); break;
      case STATE_PROCESS_LED_COMMANDS: debugPrint("Etat: Cmd LEDs"); break;
      case STATE_ERROR: debugPrint("Etat: ERREUR"); break;
      case STATE_SLEEP: break;
    }
  }
  #endif

  switch (currentState) {
  case STATE_INIT:
    // Initialise les composants un par un
    if (!sensorConnected) {
      sensorConnected = initSensor();
      if (!sensorConnected) { currentState = STATE_ERROR; break; }
    }
    if (!ledsConnected) {
      ledsConnected = initLEDs();
      if (!ledsConnected) { currentState = STATE_ERROR; break; }
    }
    if (!zigbeeConnected) {
      zigbeeConnected = initZigbeeNetwork();
      if (!zigbeeConnected) { currentState = STATE_ERROR; break; }
    }
    debugPrint("Systeme initialise!");
    showStartupAnimation();
    currentState = STATE_READ_SENSOR;
    stateTimer = millis();
    break;

  case STATE_READ_SENSOR:
    if (readSensorData()) {
      currentState = shouldSendData() ? STATE_SEND_ZIGBEE : STATE_UPDATE_LEDS;
    } else {
      if (++retryCount >= MAX_RETRY_COUNT) currentState = STATE_ERROR;
    }
    break;

  case STATE_SEND_ZIGBEE:
    debugPrint("Envoi Zigbee...");
    if (sendZigbeeData()) {
      retryCount = 0;
      currentState = STATE_UPDATE_LEDS;
    } else {
      currentState = (++retryCount >= MAX_RETRY_COUNT) ? STATE_ERROR : STATE_READ_SENSOR;
    }
    break;

  case STATE_UPDATE_LEDS:
    // État de mise à jour de l'affichage LED
    updateLEDs();
    currentState = STATE_PROCESS_LED_COMMANDS;
    break;

  case STATE_PROCESS_LED_COMMANDS:
    // État de traitement des commandes reçues via Zigbee
    processLEDCommands();
    currentState = STATE_SLEEP;
    break;

  case STATE_ERROR:
    if (errorCount < 65535) errorCount++;  // Éviter le débordement
    #if DEBUG_MODE
    debugPrintValue("ERREUR #", errorCount, "");
    #endif

    // LEDs rouges clignotantes pour signaler l'erreur
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, (millis() / 100) % 2 ? 0xFF0000 : 0);
    }
    strip.show();

    // Si trop d'erreurs, redémarrage complet du système
    if (errorCount > 10) {
      Serial.println("Trop d'erreurs - redemarrage...");
      ESP.restart();
    }

    delay(5000);  // Pause avant tentative de récupération
    sensorConnected = false;
    ledsConnected = false;
    zigbeeConnected = false;
    retryCount = 0;
    currentState = STATE_INIT;
    break;

  case STATE_SLEEP:
    // État de veille non-bloquante entre les mesures
    // Permet de continuer à traiter les commandes Zigbee pendant la veille
    if (sleepStartTime == 0) {
      sleepStartTime = millis();  // Initialisation du timer de sommeil
    }
    
    // Traite les commandes LED même pendant la veille pour rester réactif
    processLEDCommands();
    
    // Vérifie si la durée de veille est écoulée
    if (millis() - sleepStartTime >= SLEEP_DURATION) {
      sleepStartTime = 0;  // Réinitialisation du timer
      currentState = STATE_READ_SENSOR;  // Retour à la lecture du capteur
    }
    break;
  }

  // Système de watchdog pour éviter les blocages
  #if DEBUG_MODE
  if (millis() - stateTimer > WATCHDOG_TIMEOUT) {
    debugPrint("Watchdog timeout - redemarrage");
    currentState = STATE_INIT;
    stateTimer = millis();
  }
  #else
  if (millis() - stateTimer > WATCHDOG_TIMEOUT) {
    currentState = STATE_INIT;
    stateTimer = millis();
  }
  #endif
}
