#ifndef ZIGBEE_CONFIG_H
#define ZIGBEE_CONFIG_H

// Configuration réseau Zigbee
#define ZIGBEE_PAN_ID 0x1234
#define ZIGBEE_CHANNEL 11
#define ZIGBEE_ENDPOINT 1           // Point de terminaison pour température
#define ZIGBEE_INTERVAL 30000       // Intervalle d'envoi en ms (30 secondes)

// Configuration du capteur de température
#define TEMP_MEASUREMENT_INTERVAL_MS 10000  // 10 secondes
#define TEMP_REPORT_MIN_CHANGE 0.5          // Changement minimum en °C pour déclencher un rapport

// Clés de sécurité (à personnaliser pour la production)
static const uint8_t zigbee_extended_pan_id[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
static const uint8_t zigbee_network_key[] = {
    0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F, 
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0D
};

// Configuration du device Zigbee
#define ZIGBEE_DEVICE_ID ESP_ZB_HA_DEVICE_ID_TEMPERATURE_SENSOR
#define ZIGBEE_PROFILE_ID ESP_ZB_PROFILE_ID_HOME_AUTOMATION

// Configuration du cluster de température
#define ZIGBEE_TEMP_CLUSTER_ID ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT
#define ZIGBEE_TEMP_ATTR_ID ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID

#endif // ZIGBEE_CONFIG_H
