/*
 * Utilitaires de débogage pour le capteur SHT31 Zigbee
 * Fichier à inclure dans le projet Arduino
 */

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>

// Niveaux de log
enum LogLevel {
  LOG_ERROR = 0,
  LOG_WARN  = 1,
  LOG_INFO  = 2,
  LOG_DEBUG = 3
};

// Configuration du debug
#define DEBUG_LEVEL LOG_INFO
#define SERIAL_BAUD 115200

// Macros de logging
#define LOG_ERROR(msg) do { if (DEBUG_LEVEL >= LOG_ERROR) Serial.println("[ERROR] " + String(msg)); } while(0)
#define LOG_WARN(msg)  do { if (DEBUG_LEVEL >= LOG_WARN)  Serial.println("[WARN] "  + String(msg)); } while(0)
#define LOG_INFO(msg)  do { if (DEBUG_LEVEL >= LOG_INFO)  Serial.println("[INFO] "  + String(msg)); } while(0)
#define LOG_DEBUG(msg) do { if (DEBUG_LEVEL >= LOG_DEBUG) Serial.println("[DEBUG] " + String(msg)); } while(0)

// Macros de logging avec formatage
#define LOG_ERROR_F(fmt, ...) do { if (DEBUG_LEVEL >= LOG_ERROR) { Serial.print("[ERROR] "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_WARN_F(fmt, ...)  do { if (DEBUG_LEVEL >= LOG_WARN)  { Serial.print("[WARN] ");  Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_INFO_F(fmt, ...)  do { if (DEBUG_LEVEL >= LOG_INFO)  { Serial.print("[INFO] ");  Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)
#define LOG_DEBUG_F(fmt, ...) do { if (DEBUG_LEVEL >= LOG_DEBUG) { Serial.print("[DEBUG] "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } } while(0)

// Fonctions utilitaires
class DebugUtils {
public:
  // Initialisation du port série
  static void init() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial && millis() < 3000) {
      delay(10);
    }
    LOG_INFO("Debug utils initialisé");
  }

  // Affichage des informations système
  static void printSystemInfo() {
    LOG_INFO_F("Chip ID: %s", ESP.getChipModel());
    LOG_INFO_F("Flash size: %d bytes", ESP.getFlashChipSize());
    LOG_INFO_F("Free heap: %d bytes", ESP.getFreeHeap());
    LOG_INFO_F("CPU freq: %d MHz", ESP.getCpuFreqMHz());
  }

  // Affichage des informations réseau
  static void printNetworkInfo() {
    LOG_INFO("Informations réseau Zigbee:");
    // Ajouter les informations Zigbee spécifiques si disponibles
  }

  // Test de connexion I2C
  static bool testI2CConnection(int sdaPin, int sclPin) {
    LOG_INFO_F("Test de connexion I2C (SDA: %d, SCL: %d)", sdaPin, sclPin);
    
    Wire.begin(sdaPin, sclPin);
    
    // Scanner les adresses I2C
    byte error, address;
    int nDevices = 0;
    
    LOG_INFO("Scan des adresses I2C:");
    for(address = 1; address < 127; address++) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      
      if (error == 0) {
        LOG_INFO_F("  Appareil trouvé à l'adresse 0x%02X", address);
        nDevices++;
      }
    }
    
    if (nDevices == 0) {
      LOG_ERROR("Aucun appareil I2C trouvé!");
      return false;
    } else {
      LOG_INFO_F("%d appareil(s) I2C trouvé(s)", nDevices);
      return true;
    }
  }

  // Vérification de la mémoire
  static void checkMemory() {
    LOG_INFO_F("Mémoire libre: %d bytes", ESP.getFreeHeap());
    LOG_INFO_F("Mémoire minimale disponible: %d bytes", ESP.getMinFreeHeap());
    
    if (ESP.getFreeHeap() < 1000) {
      LOG_WARN("Mémoire faible!");
    }
  }

  // Gestionnaire d'erreurs
  static void handleError(const String& error, bool restart = false) {
    LOG_ERROR("ERREUR: " + error);
    
    if (restart) {
      LOG_WARN("Redémarrage du système...");
      delay(1000);
      ESP.restart();
    }
  }

  // Surveillance du watchdog
  static void setupWatchdog() {
    LOG_INFO("Configuration du watchdog...");
    // Configuration du watchdog si nécessaire
  }

  // Affichage du temps de fonctionnement
  static void printUptime() {
    unsigned long uptime = millis();
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    LOG_INFO_F("Uptime: %02d jours %02d:%02d:%02d", 
               days, hours % 24, minutes % 60, seconds % 60);
  }
};

#endif // DEBUG_UTILS_H
