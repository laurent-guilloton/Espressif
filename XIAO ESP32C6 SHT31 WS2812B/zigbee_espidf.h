#ifndef ZIGBEE_ESPIDF_H
#define ZIGBEE_ESPIDF_H

#include "esp_zigbee_core.h"
#include "esp_zigbee_zcl_command.h"
#include "esp_zigbee_ha_standard.h"
#include "esp_log.h"
#include <Arduino.h>

// Wrapper ESP-IDF Zigbee pour compatibilité Arduino
class Zigbee {
private:
    bool initialized;
    
public:
    Zigbee() : initialized(false) {}
    ~Zigbee() {}
    
    bool begin() {
        esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZC_CONFIG();
        esp_zb_init(&zb_nwk_cfg);
        esp_zb_set_pan_id(ZIGBEE_PAN_ID);
        esp_zb_set_channel(ZIGBEE_CHANNEL);
        esp_zb_start();
        initialized = true;
        Serial.println("Zigbee ESP-IDF initialisé");
        return true;
    }
    
    void setPanId(uint16_t panId) {
        esp_zb_set_pan_id(panId);
    }
    
    void setChannel(uint8_t channel) {
        esp_zb_set_channel(channel);
    }
    
    void setCoordinator(bool isCoordinator) {
        // ESP32-C6 configuré comme coordinateur
    }
    
    bool addDevice(void* device) {
        if (!initialized) return false;
        // Implémentation à compléter selon device
        return true;
    }
    
    void loop() {
        if (initialized) {
            esp_zb_main_loop_iteration();
        }
    }
};

// Classes Zigbee HA avec ESP-IDF
class ZigbeeHATemperatureSensor {
private:
    uint8_t endpoint;
    float temperature;
    esp_zb_ep_list_t *ep_list;
    
public:
    ZigbeeHATemperatureSensor(uint8_t ep) : endpoint(ep), temperature(0.0), ep_list(nullptr) {}
    
    void setDeviceId(const String& id) {}
    void setManufacturer(const String& mfg) {}
    void setModel(const String& model) {}
    void setVersion(const String& version) {}
    void setName(const String& name) {}
    
    void setTemperature(float temp) {
        temperature = temp;
    }
    
    bool report() {
        Serial.print("Température Zigbee: ");
        Serial.print(temperature);
        Serial.println("°C");
        // TODO: Implémentation ESP-IDF Zigbee
        return true;
    }
};

class ZigbeeHAHumiditySensor {
private:
    uint8_t endpoint;
    float humidity;
    
public:
    ZigbeeHAHumiditySensor(uint8_t ep) : endpoint(ep), humidity(0.0) {}
    
    void setDeviceId(const String& id) {}
    void setManufacturer(const String& mfg) {}
    void setModel(const String& model) {}
    void setVersion(const String& version) {}
    void setName(const String& name) {}
    
    void setHumidity(float hum) {
        humidity = hum;
    }
    
    bool report() {
        Serial.print("Humidité Zigbee: ");
        Serial.print(humidity);
        Serial.println("%");
        // TODO: Implémentation ESP-IDF Zigbee
        return true;
    }
};

class ZigbeeHADimmableLight {
private:
    uint8_t endpoint;
    bool isOn;
    uint8_t brightness;
    uint32_t color;
    bool stateChanged;
    
public:
    ZigbeeHADimmableLight(uint8_t ep) : endpoint(ep), isOn(false), brightness(50), color(0xFFFFFF), stateChanged(false) {}
    
    void setDeviceId(const String& id) {}
    void setManufacturer(const String& mfg) {}
    void setModel(const String& model) {}
    void setVersion(const String& version) {}
    void setName(const String& name) {}
    
    void setOn(bool on) {
        if (isOn != on) {
            isOn = on;
            stateChanged = true;
        }
    }
    
    void setBrightness(uint8_t bright) {
        if (brightness != bright) {
            brightness = bright;
            stateChanged = true;
        }
    }
    
    void setColor(uint32_t col) {
        if (color != col) {
            color = col;
            stateChanged = true;
        }
    }
    
    bool isOn() const { return isOn; }
    uint8_t getBrightness() const { return brightness; }
    uint32_t getColor() const { return color; }
    
    bool isStateChanged() {
        bool changed = stateChanged;
        stateChanged = false;
        return changed;
    }
    
    bool report() {
        Serial.println("LED Zigbee: état envoyé");
        // TODO: Implémentation ESP-IDF Zigbee
        return true;
    }
};

#endif
