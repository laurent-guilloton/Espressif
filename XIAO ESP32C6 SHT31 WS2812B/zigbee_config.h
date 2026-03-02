#ifndef ZIGBEE_CONFIG_H
#define ZIGBEE_CONFIG_H

// Configuration réseau Zigbee
#define ZIGBEE_PAN_ID 0x1234        // ID du réseau personnel
#define ZIGBEE_CHANNEL 11           // Canal WiFi (11-26)
#define ZIGBEE_ENDPOINT 1           // Point de terminaison pour température
#define ZIGBEE_ENDPOINT_HUM 2       // Point de terminaison pour humidité
#define ZIGBEE_ENDPOINT_LED 3       // Point de terminaison pour LEDs
#define ZIGBEE_INTERVAL 30000       // Intervalle d'envoi en ms (30 secondes)

// Configuration sécurité (optionnel)
#define ZIGBEE_SECURITY_ENABLED false
#define ZIGBEE_NETWORK_KEY "1234567890ABCDEF"

// Configuration device
#define DEVICE_NAME "XIAO_SHT31"
#define DEVICE_MANUFACTURER "Seeed Studio"
#define DEVICE_MODEL "XIAO ESP32-C6"
#define DEVICE_VERSION "2.0"

// Configuration LED
#define LED_MIN_BRIGHTNESS 1         // Luminosité minimale (1-100)
#define LED_MAX_BRIGHTNESS 100       // Luminosité maximale (1-100)
#define LED_DEFAULT_BRIGHTNESS 50    // Luminosité par défaut

#endif
