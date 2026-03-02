#ifndef ZIGBEE_WRAPPER_H
#define ZIGBEE_WRAPPER_H

#include <Arduino.h>
#include <stdint.h>

// Wrapper simplifié pour compatibilité avec le code existant
class Zigbee {
public:
    Zigbee() {}
    ~Zigbee() {}
    
    bool begin() {
        Serial.println("Zigbee: Initialisation...");
        // TODO: Implémentation Zigbee réelle avec ESP-IDF
        return true;
    }
    
    void setPanId(uint16_t panId) {
        Serial.print("Zigbee: PAN ID = ");
        Serial.println(panId, HEX);
    }
    
    void setChannel(uint8_t channel) {
        Serial.print("Zigbee: Canal = ");
        Serial.println(channel);
    }
    
    void setCoordinator(bool isCoordinator) {
        Serial.print("Zigbee: Coordinateur = ");
        Serial.println(isCoordinator ? "Oui" : "Non");
    }
    
    bool addDevice(void* device) {
        Serial.println("Zigbee: Device ajouté");
        return true;
    }
    
    void loop() {
        // TODO: Traiter les messages Zigbee
    }
};

// Classes pour les capteurs virtuels
class ZigbeeHATemperatureSensor {
private:
    uint8_t endpoint;
    float temperature;
    
public:
    ZigbeeHATemperatureSensor(uint8_t ep) : endpoint(ep), temperature(0.0) {}
    
    void setDeviceId(const String& id) {
        Serial.print("Temp Sensor ID: ");
        Serial.println(id);
    }
    void setManufacturer(const String& mfg) {
        Serial.print("Temp Sensor MFG: ");
        Serial.println(mfg);
    }
    void setModel(const String& model) {
        Serial.print("Temp Sensor Model: ");
        Serial.println(model);
    }
    void setVersion(const String& version) {
        Serial.print("Temp Sensor Version: ");
        Serial.println(version);
    }
    void setName(const String& name) {
        Serial.print("Temp Sensor Name: ");
        Serial.println(name);
    }
    
    void setTemperature(float temp) {
        temperature = temp;
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println("°C");
    }
    
    bool report() {
        Serial.println("Temperature envoyée via Zigbee");
        return true;
    }
};

class ZigbeeHAHumiditySensor {
private:
    uint8_t endpoint;
    float humidity;
    
public:
    ZigbeeHAHumiditySensor(uint8_t ep) : endpoint(ep), humidity(0.0) {}
    
    void setDeviceId(const String& id) {
        Serial.print("Hum Sensor ID: ");
        Serial.println(id);
    }
    void setManufacturer(const String& mfg) {
        Serial.print("Hum Sensor MFG: ");
        Serial.println(mfg);
    }
    void setModel(const String& model) {
        Serial.print("Hum Sensor Model: ");
        Serial.println(model);
    }
    void setVersion(const String& version) {
        Serial.print("Hum Sensor Version: ");
        Serial.println(version);
    }
    void setName(const String& name) {
        Serial.print("Hum Sensor Name: ");
        Serial.println(name);
    }
    
    void setHumidity(float hum) {
        humidity = hum;
        Serial.print("Humidité: ");
        Serial.print(hum);
        Serial.println("%");
    }
    
    bool report() {
        Serial.println("Humidité envoyée via Zigbee");
        return true;
    }
};

class ZigbeeHADimmableLight {
private:
    uint8_t endpoint;
    bool _isOn;
    uint8_t brightness;
    uint32_t color;
    bool stateChanged;
    
public:
    ZigbeeHADimmableLight(uint8_t ep) : endpoint(ep), _isOn(false), brightness(50), color(0xFFFFFF), stateChanged(false) {}
    
    void setDeviceId(const String& id) {
        Serial.print("LED Controller ID: ");
        Serial.println(id);
    }
    void setManufacturer(const String& mfg) {
        Serial.print("LED Controller MFG: ");
        Serial.println(mfg);
    }
    void setModel(const String& model) {
        Serial.print("LED Controller Model: ");
        Serial.println(model);
    }
    void setVersion(const String& version) {
        Serial.print("LED Controller Version: ");
        Serial.println(version);
    }
    void setName(const String& name) {
        Serial.print("LED Controller Name: ");
        Serial.println(name);
    }
    
    void setOn(bool on) {
        if (_isOn != on) {
            _isOn = on;
            stateChanged = true;
            Serial.print("LED ON: ");
            Serial.println(on);
        }
    }
    
    void setBrightness(uint8_t bright) {
        if (brightness != bright) {
            brightness = bright;
            stateChanged = true;
            Serial.print("LED Brightness: ");
            Serial.println(bright);
        }
    }
    
    void setColor(uint32_t col) {
        if (color != col) {
            color = col;
            stateChanged = true;
            Serial.print("LED Color: 0x");
            Serial.println(col, HEX);
        }
    }
    
    bool isOn() const { return _isOn; }
    uint8_t getBrightness() const { return brightness; }
    uint32_t getColor() const { return color; }
    
    bool isStateChanged() {
        bool changed = stateChanged;
        stateChanged = false;
        return changed;
    }
    
    bool report() {
        Serial.println("État LED envoyé via Zigbee");
        return true;
    }
};

#endif
