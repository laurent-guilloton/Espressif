/*
 * Xiao ESP32-C6 avec capteur SHT31 pour Home Assistant via Zigbee - VERSION OPTIMISÉE
 * 
 * Optimisations:
 * - Mode deep sleep pour économie d'énergie (batterie longue durée)
 * - Gestion mémoire optimisée (types compacts, structure unique)
 * - Communication Zigbee efficace (rapports groupés, I2C高速)
 * - Gestion d'erreurs robuste (machine à états, retry automatique)
 * - Mesures adaptatives (mode rapide si changements importants)
 * 
 * Architecture: Machine à états avec 5 états principaux
 * - STATE_INIT: Initialisation des capteurs et Zigbee
 * - STATE_READ_SENSOR: Lecture des données SHT31
 * - STATE_TRANSMIT: Envoi des données via Zigbee
 * - STATE_SLEEP: Deep sleep pour économie d'énergie
 * - STATE_ERROR: Gestion des erreurs et récupération
 */

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Zigbee.h>
#include <ZigbeeCluster.h>
#include <ZigbeeDevice.h>
#include <esp_sleep.h>
#include <Adafruit_NeoPixel.h>

// =============================================================================
// CONFIGURATION MATÉRIELLE
// =============================================================================

// Broches I2C pour le Xiao ESP32-C6
#define SDA_PIN 5
#define SCL_PIN 6

// Configuration du ruban LED WS2812B
#define LED_PIN 2              // Port D2 pour le ruban LED
#define LED_COUNT 26           // Nombre de LEDs dans le ruban
#define LED_BRIGHTNESS 50      // Luminosité par défaut (0-255)

// Configuration réseau Zigbee
#define ZIGBEE_CHANNEL 11        // Canal Zigbee (doit correspondre à Home Assistant)
#define ZIGBEE_PAN_ID 0x1234     // ID du réseau personnel

// =============================================================================
// CONFIGURATION DES INTERVALLES (en millisecondes)
// =============================================================================

#define NORMAL_READ_INTERVAL 60000    // 1 minute en mode normal (économie d'énergie)
#define FAST_READ_INTERVAL 10000      // 10 secondes si changement rapide (réactivité)
#define DEEP_SLEEP_DURATION 30        // 30 secondes de deep sleep entre les mesures
#define ERROR_RETRY_DELAY 5000        // 5 secondes en cas d'erreur (retry)

// =============================================================================
// CONFIGURATION DU MODE DEBUG
// =============================================================================

// Active/désactive le mode debug (mettre à false en production)
#define DEBUG_MODE true

// Niveaux de debug disponibles
#define DEBUG_LEVEL_NONE    0
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_WARN    2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_VERBOSE 4

// Niveau de debug actuel (modifiable selon besoin)
#define CURRENT_DEBUG_LEVEL DEBUG_LEVEL_VERBOSE

// Macros de debug pour contrôler l'affichage
#define DEBUG_ERROR(msg)   do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)   { Serial.print("[ERROR] ");   Serial.println(msg); } } while(0)
#define DEBUG_WARN(msg)    do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_WARN)    { Serial.print("[WARN] ");    Serial.println(msg); } } while(0)
#define DEBUG_INFO(msg)    do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_INFO)    { Serial.print("[INFO] ");    Serial.println(msg); } } while(0)
#define DEBUG_VERBOSE(msg) do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE) { Serial.print("[VERBOSE] "); Serial.println(msg); } } while(0)

// Macros de debug avec formatage
#define DEBUG_ERROR_F(fmt, ...)   do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_ERROR)   { Serial.print("[ERROR] ");   Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)
#define DEBUG_WARN_F(fmt, ...)    do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_WARN)    { Serial.print("[WARN] ");    Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)
#define DEBUG_INFO_F(fmt, ...)    do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_INFO)    { Serial.print("[INFO] ");    Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)
#define DEBUG_VERBOSE_F(fmt, ...) do { if (DEBUG_MODE && CURRENT_DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE) { Serial.print("[VERBOSE] "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

// =============================================================================
// SEUILS DE DÉTECTION DE CHANGEMENTS
// =============================================================================

#define TEMP_CHANGE_THRESHOLD 0.5      // 0.5°C pour activer le mode rapide
#define HUMIDITY_CHANGE_THRESHOLD 2.0  // 2% d'humidité pour activer le mode rapide

// =============================================================================
// ÉTATS DU SYSTÈME (Machine à états)
// =============================================================================

enum SystemState {
  STATE_INIT,        // Initialisation des composants
  STATE_READ_SENSOR, // Lecture du capteur SHT31
  STATE_TRANSMIT,    // Transmission des données Zigbee
  STATE_SLEEP,       // Deep sleep pour économie d'énergie
  STATE_ERROR         // Gestion des erreurs
};

// =============================================================================
// MODES POUR LE RUBAN LED
// =============================================================================

enum LedMode {
  LED_OFF,            // LEDs éteintes
  LED_SOLID,          // Couleur fixe
  LED_RAINBOW,        // Arc-en-ciel
  LED_TEMPERATURE,    // Indicateur de température
  LED_HUMIDITY,       // Indicateur d'humidité
  LED_BREATHING,      // Effet de respiration
  LED_FLASH,          // Clignotement
  LED_CHASE           // Effet de poursuite
};

// =============================================================================
// OBJETS GLOBAUX (Optimisés pour la mémoire)
// =============================================================================

// Capteur SHT31
Adafruit_SHT31 sht31;

// Ruban LED WS2812B
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Composants Zigbee
ZigbeeDevice zigbeeDevice;
ZigbeeCluster temperatureCluster;   // Cluster pour les mesures de température
ZigbeeCluster humidityCluster;      // Cluster pour les mesures d'humidité
ZigbeeCluster ledCluster;          // Cluster pour le contrôle des LEDs

// =============================================================================
// VARIABLES GLOBALES (Types optimisés)
// =============================================================================

volatile SystemState currentState = STATE_INIT;  // État actuel du système

// Dernières valeurs lues (pour détection de changements)
float lastTemperature = -999.0;  // Valeur invalide au démarrage
float lastHumidity = -999.0;     // Valeur invalide au démarrage

// Gestion du temps
unsigned long lastReadTime = 0;      // Timestamp de dernière lecture
unsigned long lastTransmitTime = 0;  // Timestamp de dernière transmission

// Gestion des erreurs
uint8_t errorCount = 0;             // Compteur d'erreurs consécutives

// Mode de fonctionnement adaptatif
bool fastMode = false;               // true = mesures rapides, false = normal

// Variables pour le contrôle des LEDs
LedMode currentLedMode = LED_OFF;    // Mode actuel des LEDs
uint8_t currentRed = 0;              // Composante rouge (0-255)
uint8_t currentGreen = 0;            // Composante verte (0-255)
uint8_t currentBlue = 0;             // Composante bleue (0-255)
uint8_t currentBrightness = LED_BRIGHTNESS; // Luminosité actuelle
unsigned long lastLedUpdate = 0;      // Timestamp dernière mise à jour LEDs
uint16_t rainbowHue = 0;             // Teinte pour arc-en-ciel
uint8_t breathingStep = 0;            // Étape pour effet respiration
uint8_t chasePosition = 0;           // Position pour effet poursuite

// =============================================================================
// STRUCTURE DE DONNÉES COMPACTE (Optimisation mémoire)
// =============================================================================

struct SensorData {
  int16_t temperature;    // Température en 0.01°C (ex: 2345 = 23.45°C)
  uint16_t humidity;      // Humidité en 0.01% (ex: 4567 = 45.67%)
  uint32_t timestamp;     // Timestamp de la mesure
  bool valid;            // Flag de validité des données
} currentData;            // Instance globale des données actuelles

// =============================================================================
// FONCTIONS UTILITAIRES LED
// =============================================================================

/**
 * Initialise le ruban LED WS2812B
 */
void initLedStrip() {
  DEBUG_INFO("🌈 Initialisation du ruban LED WS2812B...");
  
  strip.begin();
  strip.setBrightness(currentBrightness);
  strip.clear();
  strip.show();
  
  DEBUG_INFO_F("✅ Ruban LED initialisé: %d LEDs sur broche %d", LED_COUNT, LED_PIN);
  
  // Animation de démarrage
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
    strip.show();
    delay(20);
  }
  delay(500);
  strip.clear();
  strip.show();
}

/**
 * Convertit la température en couleur (bleu=froid, rouge=chaud)
 * @param temp Température en °C
 * @return Couleur RGB 32-bit
 */
uint32_t temperatureToColor(float temp) {
  uint8_t r, g, b;
  
  if (temp < 15.0) {
    // Très froid : bleu
    r = 0; g = 0; b = 255;
  } else if (temp < 20.0) {
    // Froid : cyan
    r = 0; g = 128; b = 255;
  } else if (temp < 25.0) {
    // Température : vert
    r = 0; g = 255; b = 0;
  } else if (temp < 30.0) {
    // Chaud : jaune
    r = 255; g = 255; b = 0;
  } else {
    // Très chaud : rouge
    r = 255; g = 0; b = 0;
  }
  
  return strip.Color(r, g, b);
}

/**
 * Convertit l'humidité en couleur (jaune=sec, bleu=humide)
 * @param humidity Humidité en %
 * @return Couleur RGB 32-bit
 */
uint32_t humidityToColor(float humidity) {
  uint8_t r, g, b;
  
  if (humidity < 30.0) {
    // Très sec : jaune
    r = 255; g = 255; b = 0;
  } else if (humidity < 50.0) {
    // Sec : vert
    r = 0; g = 255; b = 0;
  } else if (humidity < 70.0) {
    // Humide : cyan
    r = 0; g = 128; b = 255;
  } else {
    // Très humide : bleu
    r = 0; g = 0; b = 255;
  }
  
  return strip.Color(r, g, b);
}

/**
 * Met à jour les LEDs selon le mode actuel
 */
void updateLeds() {
  unsigned long currentTime = millis();
  
  // Mise à jour seulement toutes les 50ms pour économie d'énergie
  if (currentTime - lastLedUpdate < 50) return;
  lastLedUpdate = currentTime;
  
  switch (currentLedMode) {
    case LED_OFF:
      strip.clear();
      break;
      
    case LED_SOLID:
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
      }
      break;
      
    case LED_RAINBOW:
      for (int i = 0; i < LED_COUNT; i++) {
        uint16_t hue = (rainbowHue + (i * 360 / LED_COUNT)) % 360;
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue * 65535 / 360)));
      }
      rainbowHue = (rainbowHue + 2) % 360;
      break;
      
    case LED_TEMPERATURE:
      if (lastTemperature > -999) {
        uint32_t color = temperatureToColor(lastTemperature);
        for (int i = 0; i < LED_COUNT; i++) {
          strip.setPixelColor(i, color);
        }
      }
      break;
      
    case LED_HUMIDITY:
      if (lastHumidity > -999) {
        uint32_t color = humidityToColor(lastHumidity);
        for (int i = 0; i < LED_COUNT; i++) {
          strip.setPixelColor(i, color);
        }
      }
      break;
      
    case LED_BREATHING:
      {
        uint8_t brightness = (sin(breathingStep * 0.1) + 1) * 127;
        for (int i = 0; i < LED_COUNT; i++) {
          strip.setPixelColor(i, strip.Color(
            (currentRed * brightness) / 255,
            (currentGreen * brightness) / 255,
            (currentBlue * brightness) / 255
          ));
        }
        breathingStep = (breathingStep + 1) % 63;
      }
      break;
      
    case LED_FLASH:
      {
        bool flashState = (currentTime / 500) % 2;
        if (flashState) {
          for (int i = 0; i < LED_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
          }
        } else {
          strip.clear();
        }
      }
      break;
      
    case LED_CHASE:
      strip.clear();
      for (int i = 0; i < 5; i++) {
        int pos = (chasePosition + i) % LED_COUNT;
        uint8_t brightness = 255 - (i * 50);
        strip.setPixelColor(pos, strip.Color(
          (currentRed * brightness) / 255,
          (currentGreen * brightness) / 255,
          (currentBlue * brightness) / 255
        ));
      }
      chasePosition = (chasePosition + 1) % LED_COUNT;
      break;
  }
  
  strip.show();
}

/**
 * Définit le mode des LEDs
 * @param mode Nouveau mode LED
 * @param r Composante rouge (0-255)
 * @param g Composante verte (0-255)
 * @param b Composante bleue (0-255)
 * @param brightness Luminosité (0-255)
 */
void setLedMode(LedMode mode, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t brightness = LED_BRIGHTNESS) {
  currentLedMode = mode;
  currentRed = r;
  currentGreen = g;
  currentBlue = b;
  currentBrightness = brightness;
  
  strip.setBrightness(brightness);
  
  DEBUG_VERBOSE_F("LED mode: %d, RGB(%d,%d,%d), Brightness: %d", 
                  mode, r, g, b, brightness);
}

// =============================================================================
// FONCTIONS UTILITAIRES DEBUG
// =============================================================================

/**
 * Affiche les informations système en mode debug
 */
void printSystemInfo() {
  if (!DEBUG_MODE) return;
  
  DEBUG_INFO("=== INFORMATIONS SYSTÈME ===");
  DEBUG_INFO_F("Chip model: %s", ESP.getChipModel());
  DEBUG_INFO_F("Flash size: %d bytes", ESP.getFlashChipSize());
  DEBUG_INFO_F("Free heap: %d bytes", ESP.getFreeHeap());
  DEBUG_INFO_F("CPU freq: %d MHz", ESP.getCpuFreqMHz());
  DEBUG_INFO_F("Uptime: %lu ms", millis());
  DEBUG_INFO("========================");
}

/**
 * Affiche les informations de mémoire en mode debug
 */
void printMemoryInfo() {
  if (!DEBUG_MODE) return;
  
  DEBUG_INFO("=== MÉMOIRE ===");
  DEBUG_INFO_F("Heap libre: %d bytes", ESP.getFreeHeap());
  DEBUG_INFO_F("Heap minimum: %d bytes", ESP.getMinFreeHeap());
  DEBUG_INFO_F("Heap max bloc: %d bytes", ESP.getMaxAllocHeap());
  DEBUG_INFO("==============");
}

/**
 * Affiche les données du capteur en mode debug
 */
void printSensorData() {
  if (!DEBUG_MODE) return;
  
  DEBUG_VERBOSE("=== DONNÉES CAPTEUR ===");
  DEBUG_VERBOSE_F("Température brute: %.2f°C", lastTemperature);
  DEBUG_VERBOSE_F("Humidité brute: %.2f%%", lastHumidity);
  DEBUG_VERBOSE_F("Température encodée: %d (0.01°C)", currentData.temperature);
  DEBUG_VERBOSE_F("Humidité encodée: %d (0.01%%)", currentData.humidity);
  DEBUG_VERBOSE_F("Timestamp: %lu", currentData.timestamp);
  DEBUG_VERBOSE_F("Validité: %s", currentData.valid ? "OK" : "INVALID");
  DEBUG_VERBOSE_F("Mode rapide: %s", fastMode ? "OUI" : "NON");
  DEBUG_VERBOSE("==================");
}

/**
 * Affiche les informations Zigbee en mode debug
 */
void printZigbeeInfo() {
  if (!DEBUG_MODE) return;
  
  DEBUG_INFO("=== ZIGBEE ===");
  DEBUG_INFO_F("Canal: %d", ZIGBEE_CHANNEL);
  DEBUG_INFO_F("PAN ID: 0x%04X", ZIGBEE_PAN_ID);
  DEBUG_INFO_F("Device state: %s", zigbeeDevice.isStarted() ? "STARTED" : "STOPPED");
  DEBUG_INFO("============");
}

/**
 * Affiche l'état actuel de la machine à états
 */
void printStateMachineState() {
  if (!DEBUG_MODE) return;
  
  const char* stateNames[] = {
    "INIT", "READ_SENSOR", "TRANSMIT", "SLEEP", "ERROR"
  };
  
  DEBUG_VERBOSE_F("Machine à états: %s (erreur #%d)", 
                  stateNames[currentState], errorCount);
}

// =============================================================================
// FONCTIONS D'INITIALISATION
// =============================================================================

/**
 * Initialise le capteur SHT31
 * @return true si succès, false si échec
 */
bool initSensor() {
  DEBUG_INFO("🔧 Initialisation du capteur SHT31...");
  
  // Initialisation I2C avec les broches configurées
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // I2C高速模式 (400kHz au lieu de 100kHz par défaut)
  DEBUG_VERBOSE_F("I2C initialisé sur SDA=%d, SCL=%d @ 400kHz", SDA_PIN, SCL_PIN);
  
  // Tentative d'initialisation du capteur à l'adresse 0x44
  if (!sht31.begin(0x44)) {
    DEBUG_ERROR("❌ Échec d'initialisation du SHT31 (adresse 0x44)");
    return false;
  }
  
  // Désactivation du heater pour économie d'énergie
  sht31.setHeater(false);
  DEBUG_INFO("✅ Capteur SHT31 initialisé avec succès (heater désactivé)");
  return true;
}

/**
 * Initialise la communication Zigbee de manière optimisée
 * @return true si succès, false si échec
 */
bool initZigbeeOptimized() {
  DEBUG_INFO("📡 Initialisation de la communication Zigbee...");
  
  // Configuration de base du device Zigbee
  zigbeeDevice.begin();
  zigbeeDevice.setChannel(ZIGBEE_CHANNEL);
  zigbeeDevice.setPanId(ZIGBEE_PAN_ID);
  DEBUG_VERBOSE_F("Zigbee configuré: canal=%d, PAN_ID=0x%04X", ZIGBEE_CHANNEL, ZIGBEE_PAN_ID);
  
  // Configuration du cluster de température (standard Zigbee)
  temperatureCluster.begin(ZCL_CLUSTER_TEMPERATURE_MEASUREMENT);
  temperatureCluster.addAttribute(ZCL_ATTRIBUTE_TEMPERATURE_VALUE, ZCL_TYPE_INT16);
  DEBUG_VERBOSE("Cluster température configuré");
  
  // Configuration du cluster d'humidité (standard Zigbee)
  humidityCluster.begin(ZCL_CLUSTER_RELATIVE_HUMIDITY);
  humidityCluster.addAttribute(ZCL_ATTRIBUTE_RELATIVE_HUMIDITY_VALUE, ZCL_TYPE_UINT16);
  DEBUG_VERBOSE("Cluster humidité configuré");
  
  // Configuration du cluster pour le contrôle des LEDs
  ledCluster.begin(ZCL_CLUSTER_COLOR_CONTROL);
  ledCluster.addAttribute(ZCL_ATTRIBUTE_COLOR_CONTROL_CURRENT_HUE, ZCL_TYPE_UINT16);
  ledCluster.addAttribute(ZCL_ATTRIBUTE_COLOR_CONTROL_CURRENT_SATURATION, ZCL_TYPE_UINT8);
  ledCluster.addAttribute(ZCL_ATTRIBUTE_COLOR_CONTROL_CURRENT_X, ZCL_TYPE_UINT16);
  ledCluster.addAttribute(ZCL_ATTRIBUTE_COLOR_CONTROL_CURRENT_Y, ZCL_TYPE_UINT16);
  ledCluster.addAttribute(ZCL_ATTRIBUTE_ON_OFF, ZCL_TYPE_BOOLEAN);
  ledCluster.addAttribute(ZCL_ATTRIBUTE_LEVEL_CONTROL_CURRENT_LEVEL, ZCL_TYPE_UINT8);
  DEBUG_VERBOSE("Cluster LED configuré");
  
  // Ajout des clusters au device
  zigbeeDevice.addCluster(temperatureCluster);
  zigbeeDevice.addCluster(humidityCluster);
  zigbeeDevice.addCluster(ledCluster);
  DEBUG_VERBOSE("Clusters ajoutés au device Zigbee");
  
  // Démarrage du device Zigbee
  bool started = zigbeeDevice.start();
  if (started) {
    DEBUG_INFO("✅ Zigbee démarré avec succès");
    printZigbeeInfo(); // Affiche les infos Zigbee en mode debug
  } else {
    DEBUG_ERROR("❌ Échec du démarrage Zigbee");
  }
  
  return started;
}

// =============================================================================
// FONCTIONS DE LECTURE CAPTEUR
// =============================================================================

/**
 * Lit les données du capteur SHT31 avec optimisation
 * @return true si lecture réussie, false si échec
 */
bool readSensorOptimized() {
  DEBUG_VERBOSE("📖 Lecture du capteur SHT31...");
  
  // Lecture brute des valeurs du capteur
  float temp = sht31.readTemperature();
  float hum = sht31.readHumidity();
  
  DEBUG_VERBOSE_F("Valeurs brutes: T=%.3f°C, H=%.3f%%", temp, hum);
  
  // Validation des données lues
  if (isnan(temp) || isnan(hum)) {
    errorCount++; // Incrémentation du compteur d'erreurs
    DEBUG_ERROR_F("❌ Lecture invalide: T=%s, H=%s (erreur #%d)", 
                  isnan(temp) ? "NaN" : "OK", 
                  isnan(hum) ? "NaN" : "OK", 
                  errorCount);
    return false;
  }
  
  // Conversion optimisée en entiers (économie de mémoire)
  currentData.temperature = (int16_t)(temp * 100);  // ex: 23.45°C → 2345
  currentData.humidity = (uint16_t)(hum * 100);    // ex: 45.67% → 4567
  currentData.timestamp = millis();
  currentData.valid = true;
  
  // Détection de changements significatifs pour le mode adaptatif
  float tempDiff = abs(temp - lastTemperature);
  float humDiff = abs(hum - lastHumidity);
  
  DEBUG_VERBOSE_F("Différences: ΔT=%.2f°C, ΔH=%.2f%%", tempDiff, humDiff);
  
  // Activation du mode rapide si les changements sont importants
  fastMode = (tempDiff > TEMP_CHANGE_THRESHOLD || humDiff > HUMIDITY_CHANGE_THRESHOLD);
  
  if (fastMode) {
    DEBUG_INFO("🚀 Mode rapide activé (changements significatifs)");
  }
  
  // Mise à jour des dernières valeurs pour la prochaine comparaison
  lastTemperature = temp;
  lastHumidity = hum;
  
  // Reset du compteur d'erreurs en cas de succès
  errorCount = 0;
  
  // Affichage des données détaillées en mode debug
  printSensorData();
  
  return true;
}

// =============================================================================
// FONCTIONS DE TRANSMISSION
// =============================================================================

/**
 * Transmet les données à Home Assistant via Zigbee de manière optimisée
 */
void transmitDataOptimized() {
  DEBUG_INFO("📤 Transmission des données via Zigbee...");
  
  // Vérification de validité des données
  if (!currentData.valid) {
    DEBUG_ERROR("❌ Données invalides, transmission annulée");
    return;
  }
  
  // Mise à jour des attributs dans les clusters Zigbee
  temperatureCluster.updateAttribute(ZCL_ATTRIBUTE_TEMPERATURE_VALUE, currentData.temperature);
  humidityCluster.updateAttribute(ZCL_ATTRIBUTE_RELATIVE_HUMIDITY_VALUE, currentData.humidity);
  
  DEBUG_VERBOSE_F("Attributs mis à jour: T=%d (0.01°C), H=%d (0.01%%)", 
                  currentData.temperature, currentData.humidity);
  
  // Rapport groupé des attributs (économie de transmissions)
  temperatureCluster.reportAttribute(ZCL_ATTRIBUTE_TEMPERATURE_VALUE);
  humidityCluster.reportAttribute(ZCL_ATTRIBUTE_RELATIVE_HUMIDITY_VALUE);
  
  // Mise à jour du timestamp de dernière transmission
  lastTransmitTime = millis();
  
  DEBUG_INFO_F("✅ Données transmises (temps: %lu ms)", lastTransmitTime);
}

// =============================================================================
// FONCTIONS DE GESTION D'ÉNERGIE
// =============================================================================

/**
 * Met le système en deep sleep pour économiser l'énergie
 * ATTENTION: Cette fonction ne retourne jamais (deep sleep)
 */
void enterDeepSleep() {
  DEBUG_INFO("😴 Entrée en deep sleep pour économie d'énergie...");
  
  // Affichage des informations avant le sleep
  printMemoryInfo();
  printSystemInfo();
  
  // Configuration du réveil après DEEP_SLEEP_DURATION secondes
  esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION * 1000000ULL);
  DEBUG_INFO_F("⏰ Réveil programmé dans %d secondes", DEEP_SLEEP_DURATION);
  
  // Arrêt propre des périphériques avant le sleep
  DEBUG_VERBOSE("🔌 Arrêt des périphériques...");
  zigbeeDevice.sleep();  // Mise en veille du module Zigbee
  Wire.end();           // Arrêt du bus I2C
  
  DEBUG_INFO("🌙 Good night! Deep sleep en cours...");
  
  // Entrée en deep sleep (le système redémarrera après le timer)
  esp_deep_sleep_start();
}

// =============================================================================
// FONCTIONS DE GESTION D'ERREURS
// =============================================================================

/**
 * Gère les erreurs de manière robuste avec retry automatique
 */
void handleErrors() {
  errorCount++;
  
  DEBUG_ERROR_F("⚠️ Erreur système #%d détectée", errorCount);
  
  // Si trop d'erreurs consécutives, redémarrage du système
  if (errorCount > 5) {
    DEBUG_ERROR("🔄 Trop d'erreurs consécutives, redémarrage forcé du système...");
    printMemoryInfo(); // Info mémoire avant redémarrage
    delay(1000);
    ESP.restart();
  }
  
  // Attente avant de réessayer (évite les boucles d'erreur rapides)
  DEBUG_WARN_F("⏳ Attente de %d ms avant retry...", ERROR_RETRY_DELAY);
  delay(ERROR_RETRY_DELAY);
  
  // Retour à l'état d'initialisation pour réessayer
  DEBUG_INFO("🔧 Retour à l'état d'initialisation");
  currentState = STATE_INIT;
}

// =============================================================================
// FONCTIONS PRINCIPALES (setup et loop)
// =============================================================================

/**
 * Fonction d'initialisation appelée au démarrage
 */
void setup() {
  // Initialisation du port série pour le debug
  Serial.begin(115200);
  delay(100); // Attente pour stabilisation du port série
  
  DEBUG_INFO("=== DÉMARRAGE DU CAPTEUR SHT31 ZIGBEE OPTIMISÉ AVEC LEDS ===");
  DEBUG_INFO_F("🔧 Mode debug: %s", DEBUG_MODE ? "ACTIVÉ" : "DÉSACTIVÉ");
  DEBUG_INFO_F("📊 Niveau debug: %d", CURRENT_DEBUG_LEVEL);
  
  // Initialisation du ruban LED
  initLedStrip();
  
  // Affichage des informations système au démarrage
  printSystemInfo();
  
  // État initial: initialisation des composants
  currentState = STATE_INIT;
  
  DEBUG_INFO("🚀 Prêt à démarrer la machine à états");
}

/**
 * Boucle principale basée sur une machine à états
 * Chaque état gère une phase spécifique du fonctionnement
 */
void loop() {
  // Affichage de l'état actuel en mode debug
  printStateMachineState();
  
  // Mise à jour des LEDs (si mode actif)
  updateLeds();
  
  // Machine à états principale
  switch (currentState) {
    
    // =======================================================================
    // ÉTAT D'INITIALISATION
    // =======================================================================
    case STATE_INIT:
      DEBUG_INFO("🔄 État: INITIALISATION des composants...");
      
      // Tentative d'initialisation du capteur et de Zigbee
      if (initSensor() && initZigbeeOptimized()) {
        currentState = STATE_READ_SENSOR;  // Succès → passage à lecture
        DEBUG_INFO("✅ Initialisation réussie, passage à lecture capteur");
        // Animation de succès
        setLedMode(LED_SOLID, 0, 255, 0, 100);
        delay(1000);
        setLedMode(LED_OFF);
      } else {
        DEBUG_ERROR("❌ Erreur d'initialisation");
        handleErrors();  // Échec → gestion d'erreur
      }
      break;
      
    // =======================================================================
    // ÉTAT DE LECTURE DU CAPTEUR
    // =======================================================================
    case STATE_READ_SENSOR:
      DEBUG_VERBOSE("📡 État: LECTURE du capteur SHT31...");
      
      if (readSensorOptimized()) {
        currentState = STATE_TRANSMIT;  // Succès → transmission
        DEBUG_INFO("✅ Lecture réussie, passage à transmission");
        
        // Affichage des informations avec mode actuel
        DEBUG_INFO_F("🌡️  Temp: %.2f°C | 💧 Hum: %.2f%% | %s", 
                     lastTemperature, lastHumidity, 
                     fastMode ? "🚀 MODE RAPIDE" : "🐢 MODE NORMAL");
      } else {
        DEBUG_ERROR("❌ Erreur lecture capteur");
        handleErrors();  // Échec → gestion d'erreur
      }
      break;
      
    // =======================================================================
    // ÉTAT DE TRANSMISSION
    // =======================================================================
    case STATE_TRANSMIT:
      DEBUG_VERBOSE("📤 État: TRANSMISSION des données via Zigbee...");
      transmitDataOptimized();
      
      // Détermination du prochain état selon le mode de fonctionnement
      if (fastMode) {
        // Mode rapide: on continue les mesures fréquentes
        currentState = STATE_READ_SENSOR;
        DEBUG_INFO_F("🚀 Mode rapide activé, prochaine mesure dans %d ms...", FAST_READ_INTERVAL);
        delay(FAST_READ_INTERVAL);
      } else {
        // Mode normal: on passe en deep sleep pour économie d'énergie
        currentState = STATE_SLEEP;
        DEBUG_INFO("🐢 Mode normal, passage en deep sleep...");
      }
      break;
      
    // =======================================================================
    // ÉTAT DE SOMMEIL (DEEP SLEEP)
    // =======================================================================
    case STATE_SLEEP:
      DEBUG_VERBOSE("😴 État: DEEP SLEEP");
      enterDeepSleep();  // Cette fonction ne retourne jamais
      break;
      
    // =======================================================================
    // ÉTAT D'ERREUR
    // =======================================================================
    case STATE_ERROR:
      DEBUG_ERROR("⚠️  État: ERREUR système détecté");
      handleErrors();
      break;
  }
  
  // Yield pour éviter le watchdog et permettre au système de respirer
  // CRUCIAL: Sans cette ligne, le système redémarre aléatoirement
  yield();
}
