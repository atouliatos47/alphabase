/*
 * ESP32 AlphaBase MQTT Example
 * 
 * This sketch shows how to:
 * 1. Connect ESP32 to WiFi
 * 2. Connect to Mosquitto MQTT Broker
 * 3. Publish sensor data to MQTT topics
 * 4. Subscribe to topics and receive commands
 * 
 * Compatible with: ESP32, ESP32-S2, ESP32-C3
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define LED_BUILTIN 2  // Built-in LED on most ESP32 boards

// WiFi credentials
const char* ssid = "SKYPL2JH";
const char* password = "zNeUN3iQa2AbCJ";

// AlphaBase/MQTT server
const char* alphabaseURL = "http://192.168.0.52:8000";  // For HTTP sketch
const char* mqttServer = "192.168.0.52";  // For MQTT sketch
const int mqttPort = 1883;
const char* mqttUser = "";      // Leave empty if no authentication
const char* mqttPassword = "";  // Leave empty if no authentication

// MQTT Topics
const char* topicSensorData = "alphabase/sensors/esp32_01";
const char* topicCommands = "alphabase/commands/esp32_01";
const char* topicStatus = "alphabase/status/esp32_01";

// Device info
const char* deviceID = "ESP32-01";

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Timing
unsigned long lastPublish = 0;
const long publishInterval = 5000;  // Publish every 5 seconds

// Function declarations
bool connectWiFi();
bool connectMQTT();
void callback(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void publishStatus(String status);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 AlphaBase MQTT Example ===\n");
  
  // Connect to WiFi
  if (!connectWiFi()) {
    Serial.println("Failed to connect to WiFi. Restarting...");
    delay(5000);
    ESP.restart();
  }
  
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
  
  // Connect to MQTT
  if (!connectMQTT()) {
    Serial.println("Failed to connect to MQTT. Check broker settings.");
  }
  
  // Publish online status
  publishStatus("online");
  
  Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    Serial.println("MQTT disconnected. Reconnecting...");
    connectMQTT();
  }
  mqttClient.loop();
  
  // Publish sensor data periodically
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublish >= publishInterval) {
    lastPublish = currentMillis;
    publishSensorData();
  }
}

// Connect to WiFi
bool connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  
  Serial.println("\n✗ WiFi Connection Failed");
  return false;
}

// Connect to MQTT Broker
bool connectMQTT() {
  Serial.print("Connecting to MQTT Broker");
  
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    Serial.print(".");
    
    // Attempt to connect
    String clientId = "ESP32-" + String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("\n✓ MQTT Connected!");
      
      // Subscribe to command topic
      mqttClient.subscribe(topicCommands);
      Serial.print("Subscribed to: ");
      Serial.println(topicCommands);
      
      return true;
    }
    
    Serial.print("\n✗ MQTT Connection Failed. State: ");
    Serial.println(mqttClient.state());
    delay(2000);
    attempts++;
  }
  
  return false;
}

// MQTT callback - receives messages from subscribed topics
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n--- MQTT Message Received ---");
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  // Parse JSON command
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Handle commands
  const char* command = doc["command"];
  
  if (strcmp(command, "restart") == 0) {
    Serial.println("Command: RESTART");
    publishStatus("restarting");
    delay(1000);
    ESP.restart();
  }
  else if (strcmp(command, "led_on") == 0) {
    Serial.println("Command: LED ON");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    publishStatus("led_on");
  }
  else if (strcmp(command, "led_off") == 0) {
    Serial.println("Command: LED OFF");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    publishStatus("led_off");
  }
  else if (strcmp(command, "status") == 0) {
    Serial.println("Command: STATUS REQUEST");
    publishStatus("active");
    publishSensorData();
  }
  else {
    Serial.print("Unknown command: ");
    Serial.println(command);
  }
}

// Publish sensor data
void publishSensorData() {
  // Simulate reading sensors (replace with real sensors)
  float temperature = random(200, 300) / 10.0;  // 20-30°C
  float humidity = random(400, 700) / 10.0;     // 40-70%
  int rssi = WiFi.RSSI();
  
  // Create JSON payload
  StaticJsonDocument<256> doc;
  doc["device_id"] = deviceID;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["rssi"] = rssi;
  doc["uptime"] = millis() / 1000;
  doc["timestamp"] = millis();
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  // Publish to MQTT
  if (mqttClient.publish(topicSensorData, jsonData.c_str())) {
    Serial.println("--- Sensor Data Published ---");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");
  } else {
    Serial.println("✗ Failed to publish sensor data");
  }
}

// Publish device status
void publishStatus(String status) {
  StaticJsonDocument<128> doc;
  doc["device_id"] = deviceID;
  doc["status"] = status;
  doc["timestamp"] = millis();
  doc["ip"] = WiFi.localIP().toString();
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  mqttClient.publish(topicStatus, jsonData.c_str());
  
  Serial.print("Status published: ");
  Serial.println(status);
}