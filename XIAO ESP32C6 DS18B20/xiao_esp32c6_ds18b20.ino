#include "zigbee_config.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Zigbee.h>
#include <ZigbeeHA.h>

// Configuration du capteur DS18B20
#define ONE_WIRE_BUS D2
#define TEMP_RESOLUTION 12
#define MAX_RETRY_COUNT 3
#define TEMP_CHANGE_THRESHOLD 0.1 // Changement minimum en °C pour envoyer
#define WATCHDOG_TIMEOUT 30000    // 30 secondes
#define TEMP_SENTINEL -999.0      // Valeur initiale impossible pour lastSentTemp

// Active/désactive les messages de débogage (0 = désactivé, 1 = activé)
#define DEBUG_MODE 1

// États du système
enum SystemState {
  STATE_INIT,
  STATE_READ_SENSOR,
  STATE_SEND_ZIGBEE,
  STATE_ERROR,
  STATE_SLEEP
};

// Création des objets
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Zigbee zigbee;
ZigbeeHATemperatureSensor tempSensor(ZIGBEE_ENDPOINT);

// Variables optimisées
volatile float temperatureC = 0.0;
volatile float lastSentTemp = TEMP_SENTINEL;
SystemState currentState = STATE_INIT;
unsigned long stateTimer = 0;
uint8_t retryCount = 0;
uint16_t errorCount = 0;
bool sensorConnected = false;
unsigned long sleepStartTime = 0;       // Timer pour la gestion non-bloquante du sommeil

// Optimisation mémoire
struct SensorData {
  float temp;
  bool valid;
  unsigned long timestamp;
} currentReading;

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
bool initSensor() {
  sensors.begin();
  uint8_t deviceCount = sensors.getDeviceCount();

  if (deviceCount == 0) {
    Serial.println("ERREUR: Aucun capteur DS18B20 détecté");
    return false;
  }

  Serial.print(deviceCount);
  Serial.println(" capteur(s) DS18B20 détecté(s)");

  sensors.setResolution(TEMP_RESOLUTION);
  sensors.setWaitForConversion(true); // Mode bloquant pour lecture fiable

  return true;
}

bool initZigbeeNetwork() {
  zigbee.setPanId(ZIGBEE_PAN_ID);
  zigbee.setChannel(ZIGBEE_CHANNEL);
  zigbee.setCoordinator(true);

  if (!zigbee.begin()) {
    Serial.println("ERREUR: Impossible de démarrer Zigbee");
    return false;
  }

  uint64_t chipId = ESP.getEfuseMac();
  String chipIdHex = String((uint32_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
  tempSensor.setDeviceId("XIAO_TEMP_" + chipIdHex);
  tempSensor.setManufacturer("Seeed Studio");
  tempSensor.setModel("XIAO ESP32-C6");
  tempSensor.setVersion("2.0");

  if (!zigbee.addDevice(&tempSensor)) {
    Serial.println("ERREUR: Impossible d'ajouter le capteur Zigbee");
    return false;
  }

  return true;
}

bool readTemperature() {
  sensors.requestTemperatures();
  // setWaitForConversion(true) assure que la conversion est terminée

  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C || temp < -55.0 || temp > 125.0) {
    Serial.println("ERREUR: Lecture invalide du capteur");
    return false;
  }

  currentReading.temp = temp;
  currentReading.valid = true;
  currentReading.timestamp = millis();

  return true;
}

bool shouldSendTemperature() {
  if (!currentReading.valid)
    return false;

  // Envoyer si premier envoi ou changement significatif
  if (lastSentTemp < (TEMP_SENTINEL + 1.0) ||
      abs(currentReading.temp - lastSentTemp) >= TEMP_CHANGE_THRESHOLD) {
    return true;
  }

  // Envoyer périodiquement même si pas de changement
  return (millis() - stateTimer) >= ZIGBEE_INTERVAL;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Xiao ESP32-C6 - Capteur DS18B20 Optimisé ===");
  Serial.println("Version 2.0 - Mode machine à états");

  currentState = STATE_INIT;
  stateTimer = millis();
  currentReading.valid = false;  // Initialisation explicite
}

void loop() {
  switch (currentState) {
  case STATE_INIT:
    Serial.println("État: Initialisation...");

    if (!sensorConnected) {
      sensorConnected = initSensor();
      if (!sensorConnected) {
        currentState = STATE_ERROR;
        break;
      }
    }

    if (!zigbeeConnected) {
      zigbeeConnected = initZigbeeNetwork();
      if (!zigbeeConnected) {
        currentState = STATE_ERROR;
        break;
      }
    }

    Serial.println("Système initialisé avec succès!");
    currentState = STATE_READ_SENSOR;
    stateTimer = millis();
    break;

  case STATE_READ_SENSOR:
    if (readTemperature()) {
      Serial.print("Température: ");
      Serial.print(currentReading.temp);
      Serial.println(" °C");

      if (shouldSendTemperature()) {
        currentState = STATE_SEND_ZIGBEE;
      } else {
        currentState = STATE_SLEEP;
      }
    } else {
      retryCount++;
      if (retryCount >= MAX_RETRY_COUNT) {
        currentState = STATE_ERROR;
      }
    }
    break;

  case STATE_SEND_ZIGBEE:
    Serial.println("Envoi via Zigbee...");

    tempSensor.setTemperature(currentReading.temp);
    if (tempSensor.report()) {
      lastSentTemp = currentReading.temp;
      Serial.println("Température envoyée avec succès");
      retryCount = 0;
      currentState = STATE_SLEEP;
    } 
    else 
    {
      Serial.println("ERREUR: Échec envoi Zigbee");
      retryCount++;
      if (retryCount >= MAX_RETRY_COUNT) {
        currentState = STATE_ERROR;
      } else {
        currentState = STATE_READ_SENSOR;
      }
    }
    break;

  case STATE_ERROR:
    errorCount++;
    Serial.print("ERREUR système (");
    Serial.print(errorCount);
    Serial.println(")");

    if (errorCount > 10) {
      Serial.println("Trop d'erreurs - redémarrage...");
      ESP.restart();
    }

    // Tentative de récupération
    delay(5000);
    sensorConnected = false;
    zigbeeConnected = false;
    retryCount = 0;
    currentState = STATE_INIT;
    break;

  case STATE_SLEEP:
    // État de veille non-bloquante entre les mesures
    if (sleepStartTime == 0) {
      sleepStartTime = millis();  // Initialisation du timer de sommeil
    }
    
    // Vérifie si la durée de veille est écoulée
    if (millis() - sleepStartTime >= ZIGBEE_INTERVAL) {
      sleepStartTime = 0;  // Réinitialisation du timer
      stateTimer = millis(); // Mise à jour du watchdog timer
      currentState = STATE_READ_SENSOR;  // Retour à la lecture du capteur
    }
    break;
  }

  // Watchdog pour éviter les blocages
  if (millis() - stateTimer > WATCHDOG_TIMEOUT) {
    Serial.println("Watchdog timeout - redémarrage état");
    currentState = STATE_INIT;
    stateTimer = millis();
  }
}
