/*
 * Xiao ESP32-C6 avec capteur SHT31 pour Home Assistant via Zigbee
 * 
 * Ce code mesure la température et l'humidité avec un capteur SHT31
 * et envoie les données à Home Assistant via le protocole Zigbee
 * 
 * Matériel requis:
 * - Xiao ESP32-C6
 * - Capteur SHT31
 * - Connexions I2C: SDA -> GPIO5, SCL -> GPIO6
 */

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Zigbee.h>
#include <ZigbeeCluster.h>
#include <ZigbeeDevice.h>

// Configuration du capteur SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Configuration Zigbee
ZigbeeDevice zigbeeDevice;
ZigbeeCluster temperatureCluster;
ZigbeeCluster humidityCluster;

// Configuration du réseau
#define ZIGBEE_CHANNEL 11
#define ZIGBEE_PAN_ID 0x1234

// Variables globales
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 30000; // Lecture toutes les 30 secondes

// Tags pour le logging
static const char *TAG = "SHT31_ZIGBEE";

// Structure pour les données du capteur
typedef struct {
  float temperature;
  float humidity;
} sensor_data_t;

// Fonction pour initialiser le capteur SHT31
bool initSHT31() {
  Serial.begin(115200);
  Serial.println("Initialisation du capteur SHT31...");
  
  // Initialisation I2C pour Xiao ESP32-C6
  Wire.begin(5, 6); // SDA=GPIO5, SCL=GPIO6
  
  if (!sht31.begin(0x44)) {  // Adresse I2C par défaut du SHT31
    Serial.println("Erreur: Impossible de trouver le capteur SHT31!");
    return false;
  }
  
  Serial.println("Capteur SHT31 initialisé avec succès!");
  return true;
}

// Fonction pour lire les données du capteur
bool readSensorData() {
  temperature = sht31.readTemperature();
  humidity = sht31.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Erreur: Lecture invalide du capteur!");
    return false;
  }
  
  Serial.printf("Température: %.2f°C, Humidité: %.2f%%\n", temperature, humidity);
  return true;
}

// Callback pour les attributs Zigbee
void zigbeeAttributeHandler(ZigbeeAttribute *attribute) {
  Serial.print("Attribut reçu: ");
  Serial.println(attribute->getId());
}

// Fonction pour initialiser Zigbee
void initZigbee() {
  Serial.println("Initialisation de Zigbee...");
  
  // Configuration du device Zigbee
  zigbeeDevice.begin();
  zigbeeDevice.setChannel(ZIGBEE_CHANNEL);
  zigbeeDevice.setPanId(ZIGBEE_PAN_ID);
  
  // Configuration du cluster de température
  temperatureCluster.begin(ZCL_CLUSTER_TEMPERATURE_MEASUREMENT);
  temperatureCluster.addAttribute(ZCL_ATTRIBUTE_TEMPERATURE_VALUE, ZCL_TYPE_INT16);
  
  // Configuration du cluster d'humidité
  humidityCluster.begin(ZCL_CLUSTER_RELATIVE_HUMIDITY);
  humidityCluster.addAttribute(ZCL_ATTRIBUTE_RELATIVE_HUMIDITY_VALUE, ZCL_TYPE_UINT16);
  
  // Ajout des clusters au device
  zigbeeDevice.addCluster(temperatureCluster);
  zigbeeDevice.addCluster(humidityCluster);
  
  // Démarrage du device
  zigbeeDevice.start();
  
  Serial.println("Zigbee démarré avec succès!");
}

// Fonction pour envoyer les données à Home Assistant
void sendToHomeAssistant() {
  // Conversion de la température en entiers (0.01°C)
  int16_t tempValue = (int16_t)(temperature * 100);
  temperatureCluster.updateAttribute(ZCL_ATTRIBUTE_TEMPERATURE_VALUE, tempValue);
  
  // Conversion de l'humidité en entiers (0.01%)
  uint16_t humidityValue = (uint16_t)(humidity * 100);
  humidityCluster.updateAttribute(ZCL_ATTRIBUTE_RELATIVE_HUMIDITY_VALUE, humidityValue);
  
  // Publication des attributs
  temperatureCluster.reportAttribute(ZCL_ATTRIBUTE_TEMPERATURE_VALUE);
  humidityCluster.reportAttribute(ZCL_ATTRIBUTE_RELATIVE_HUMIDITY_VALUE);
  
  Serial.println("Données envoyées à Home Assistant via Zigbee");
}

void setup() {
  // Initialisation du capteur SHT31
  if (!initSHT31()) {
    Serial.println("Erreur d'initialisation du capteur!");
    while (1) {
      delay(1000);
    }
  }
  
  // Initialisation de Zigbee
  initZigbee();
  
  // Première lecture
  readSensorData();
  
  Serial.println("Système prêt!");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Lecture du capteur à intervalles réguliers
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    
    if (readSensorData()) {
      sendToHomeAssistant();
    }
  }
  
  // Traitement des événements Zigbee
  zigbeeDevice.update();
  
  delay(100);
}
