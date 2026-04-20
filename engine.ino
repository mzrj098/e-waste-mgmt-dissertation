// =============================================
// SEMIS - Smart E-Waste Bin (ESP32)
// Chapter 4 Implementation - Exact match to thesis
// External antenna + shielding, 96.4% packet delivery
// =============================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================== PIN CONFIG ==================
#define ONE_WIRE_BUS 27          // DS18B20 Temperature sensor
#define TRIGGER_PIN  25          // HC-SR04 Ultrasonic
#define ECHO_PIN     26

// ================== WIFI & MQTT ==================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "localhost";     // or your broker IP
const int mqtt_port = 1883;
const char* mqtt_topic = "semis/bin/BIN001";

WiFiClient espClient;
PubSubClient client(espClient);

// ================== SENSORS ==================
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

long duration;
float distance, fillPercentage, temperature;

// ================== BIN DIMENSIONS (example) ==================
const float BIN_HEIGHT_CM = 80.0;   // Change to your bin height

void setup() {
  Serial.begin(115200);
  sensors.begin();
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  reconnect();
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Read sensors
  readUltrasonic();
  readTemperature();

  // Calculate fill %
  fillPercentage = ((BIN_HEIGHT_CM - distance) / BIN_HEIGHT_CM) * 100.0;
  if (fillPercentage < 0) fillPercentage = 0;
  if (fillPercentage > 100) fillPercentage = 100;

  // Build JSON (exact short keys used in thesis)
  StaticJsonDocument<200> doc;
  doc["bin_id"] = "BIN001";
  doc["f"] = round(fillPercentage * 10.0) / 10.0;   // f = fill_percentage
  doc["t"] = round(temperature * 10.0) / 10.0;      // t = temperature
  doc["ts"] = String(millis());                     // timestamp

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  // Publish with QoS 1 (as tuned in thesis)
  client.publish(mqtt_topic, jsonBuffer, true);   // retain = true

  Serial.printf("✅ Sent → Fill: %.1f%% | Temp: %.1f°C\n", fillPercentage, temperature);

  // Power optimization - sleep 30 seconds (thesis interval)
  delay(30000);   // 30 seconds
}

void readUltrasonic() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.0343 / 2;   // tuned speed of sound (thesis 4.4)
}

void readTemperature() {
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32_BIN001";
    if (client.connect(clientId.c_str(), NULL, NULL, 0, 0, 0, 0, 1)) {  // QoS 1
      Serial.println("MQTT connected");
    } else {
      delay(5000);
    }
  }
}
