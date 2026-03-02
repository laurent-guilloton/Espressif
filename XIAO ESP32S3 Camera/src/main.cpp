/*
 * Xiao ESP32S3 Sense Camera pour Home Assistant
 * Transmet les images de la caméra vers Home Assistant via HTTP
 * 
 * Matériel requis:
 * - Xiao ESP32S3 Sense
 * - Caméra OV2640
 * 
 * Bibliothèques requises:
 * - WiFi (incluse dans ESP32)
 * - ArduinoJson
 * - HTTPClient
 * - WebServer
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "camera_pins.h"

// Configuration WiFi
const char* ssid = "Freebox-25ECF7";
const char* password = "nmb64qznzdqfrd2zqhh973";

// Configuration Home Assistant
const char* ha_url = "http://192.168.1.237:8123";
const char* ha_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiI5NjFhMzYxMGFmMmI0MjY4YjA3NGI1ZjY1ZGMwMTFhYSIsImlhdCI6MTc3MjA1MjYyNCwiZXhwIjoyMDg3NDEyNjI0fQ.hsSYsXJbdVi1WouRVPz48M-zxFhHT6LlhvSlDxq5LB8";
const char* camera_entity_id = "camera.xiao_esp32s3";

// Configuration caméra
#define CAMERA_MODEL_XIAO_ESP32S3
#define FRAME_SIZE FRAMESIZE_VGA
#define JPEG_QUALITY 12
#define FB_COUNT 2

// Variables globales
unsigned long lastTransmission = 0;
unsigned long transmissionInterval = 1000; // 1 seconde entre les transmissions
bool cameraInitialized = false;

// Serveur web pour les endpoints directs
WebServer server(80);

// Prototypes de fonctions
void setupWiFi();
void setupCamera();
void setupWebServer();
void captureAndTransmit();
void transmitImageToHA(uint8_t* imageData, size_t imageSize);
void printStatus();

// Broches pour Xiao ESP32S3 Sense avec caméra OV2640
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15

#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

void setup() {
  Serial.begin(115200);
  Serial.println("Démarrage Xiao ESP32S3 Camera pour Home Assistant...");
  
  // Initialisation du WiFi
  setupWiFi();
  
  // Initialisation de la caméra
  setupCamera();
  
  // Démarrage du serveur web
  setupWebServer();
  
  Serial.println("Serveur web démarré sur http://" + WiFi.localIP().toString());
}

void setupWiFi() {
  Serial.println("Connexion au WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connecté!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
}

void setupCamera() {
  Serial.println("Initialisation de la caméra...");
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Initialisation avec haute résolution pour les premiers tests
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = FB_COUNT;
  
  // Initialisation de la caméra
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Échec de l'initialisation de la caméra: 0x%x\n", err);
    return;
  }
  
  cameraInitialized = true;
  Serial.println("Caméra initialisée avec succès!");
  
  // Ajustement des paramètres pour de meilleures performances
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);     // Luminosité
    s->set_contrast(s, 0);       // Contraste
    s->set_saturation(s, 0);     // Saturation
    s->set_special_effect(s, 0); // Pas d'effet spécial
    s->set_whitebal(s, 1);        // Balance des blancs automatique
    s->set_awb_gain(s, 1);        // Gain AWB automatique
    s->set_wb_mode(s, 0);         // Mode AWB automatique
    s->set_exposure_ctrl(s, 1);   // Contrôle d'exposition automatique
    s->set_aec2(s, 0);            // AEC algorithm
    s->set_ae_level(s, 0);        // Niveau AE
    s->set_aec_value(s, 300);     // Valeur AEC
    s->set_gain_ctrl(s, 1);       // Contrôle de gain automatique
    s->set_agc_gain(s, 0);        // Gain AGC
    s->set_gainceiling(s, (gainceiling_t)0); // Gain ceiling
    s->set_bpc(s, 0);             // Black pixel correction
    s->set_wpc(s, 1);             // White pixel correction
    s->set_raw_gma(s, 1);         // Raw gamma
    s->set_lenc(s, 1);            // Lens correction
    s->set_hmirror(s, 0);         // Miroir horizontal
    s->set_vflip(s, 0);           // Flip vertical
    s->set_dcw(s, 1);             // DCW (downsize)
    s->set_colorbar(s, 0);        // Barre de couleur de test
  }
}

void setupWebServer() {
  // Endpoint pour capture d'image unique
  server.on("/capture", HTTP_GET, []() {
    if (!cameraInitialized) {
      server.send(500, "text/plain", "Camera not initialized");
      return;
    }
    
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      server.send(500, "text/plain", "Camera capture failed");
      return;
    }
    
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Length", String(fb->len));
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    WiFiClient client = server.client();
    client.write("HTTP/1.1 200 OK\r\n");
    client.write("Content-Type: image/jpeg\r\n");
    String contentLength = "Content-Length: " + String(fb->len) + "\r\n";
    client.write(contentLength.c_str(), contentLength.length());
    client.write("Access-Control-Allow-Origin: *\r\n");
    client.write("\r\n");
    client.write(fb->buf, fb->len);
    
    esp_camera_fb_return(fb);
  });
  
  // Endpoint pour streaming vidéo
  server.on("/stream", HTTP_GET, []() {
    if (!cameraInitialized) {
      server.send(500, "text/plain", "Camera not initialized");
      return;
    }
    
    WiFiClient client = server.client();
    
    // En-têtes HTTP pour le streaming MJPEG
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Cache-Control: no-cache");
    client.println("Connection: close");
    client.println();
    
    // Streaming continu
    while (client.connected()) {
      camera_fb_t* fb = esp_camera_fb_get();
      if (fb) {
        client.println("--frame");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(fb->len));
        client.println();
        client.write(fb->buf, fb->len);
        client.println();
        esp_camera_fb_return(fb);
      }
      delay(50); // ~20 FPS
    }
  });
  
  // Endpoint pour le statut
  server.on("/status", HTTP_GET, []() {
    DynamicJsonDocument doc(1024);
    doc["camera_initialized"] = cameraInitialized;
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ip_address"] = WiFi.localIP().toString();
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["uptime"] = millis();
    
    String response;
    serializeJson(doc, response);
    
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
  });
  
  // Endpoint pour les commandes
  server.on("/command", HTTP_POST, []() {
    String body = server.arg("plain");
    DynamicJsonDocument doc(256);
    deserializeJson(doc, body);
    
    if (doc.containsKey("interval")) {
      transmissionInterval = doc["interval"];
      Serial.printf("Interval updated to %d ms\n", transmissionInterval);
    }
    
    if (doc.containsKey("quality")) {
      int quality = doc["quality"];
      sensor_t *s = esp_camera_sensor_get();
      if (s) {
        s->set_quality(s, quality);
      }
    }
    
    server.sendHeader("Content-Type", "application/json");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.begin();
}

void loop() {
  // Gestion du serveur web
  server.handleClient();
  
  // Vérification de la connexion WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi déconnecté, tentative de reconnexion...");
    setupWiFi();
    return;
  }
  
  // Gestion des commandes série
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "status") {
      printStatus();
    } else if (command == "capture") {
      captureAndTransmit();
    } else if (command.startsWith("interval:")) {
      int newInterval = command.substring(9).toInt();
      if (newInterval > 0) {
        transmissionInterval = newInterval;
        Serial.printf("Intervalle de transmission mis à jour: %d ms\n", newInterval);
      }
    }
  }
  
  delay(10);
}

void captureAndTransmit() {
  if (!cameraInitialized) return;
  
  // Capture d'une image
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Échec de la capture d'image");
    return;
  }
  
  // Transmission vers Home Assistant
  transmitImageToHA(fb->buf, fb->len);
  
  // Libération du buffer
  esp_camera_fb_return(fb);
}

void transmitImageToHA(uint8_t* imageData, size_t imageSize) {
  HTTPClient http;
  String url = String(ha_url) + "/api/camera_proxy/" + camera_entity_id;
  
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + String(ha_token));
  http.addHeader("Content-Type", "image/jpeg");
  
  int httpResponseCode = http.POST(imageData, imageSize);
  
  if (httpResponseCode > 0) {
    Serial.printf("Image transmise, taille: %d bytes, code: %d\n", imageSize, httpResponseCode);
  } else {
    Serial.printf("Erreur de transmission: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  
  http.end();
}

void printStatus() {
  Serial.println("=== Statut Xiao ESP32S3 Camera ===");
  Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connecté" : "Déconnecté");
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("Caméra: %s\n", cameraInitialized ? "Initialisée" : "Non initialisée");
  Serial.printf("Intervalle transmission: %d ms\n", transmissionInterval);
  Serial.printf("Signal WiFi: %d dBm\n", WiFi.RSSI());
  Serial.printf("Mémoire libre: %d bytes\n", ESP.getFreeHeap());
  Serial.println("================================");
}
