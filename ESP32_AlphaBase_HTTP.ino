/*
 * ESP32 AlphaBase HTTP Example
 * 
 * This sketch shows how to:
 * 1. Connect ESP32 to WiFi
 * 2. Authenticate with AlphaBase
 * 3. Send sensor data to AlphaBase
 * 4. Retrieve data from AlphaBase
 * 
 * Compatible with: ESP32, ESP32-S2, ESP32-C3
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "SKYPL2JH";
const char* password = "zNeUN3iQa2AbCJ";

// AlphaBase/MQTT server
const char* alphabaseURL = "http://192.168.0.52:8000";  // For HTTP sketch
const char* mqttServer = "192.168.0.52";  // For MQTT sketch

// AlphaBase credentials
const char* alphabaseUsername = "atoul";
const char* alphabasePassword = "password123";

// Authentication token (stored after login)
String authToken = "";

// Function declarations
bool connectWiFi();
bool loginAlphaBase();
bool sendSensorData(String collection, String key, float value);
bool getData(String collection, String key);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 AlphaBase HTTP Example ===\n");
  
  // Connect to WiFi
  if (!connectWiFi()) {
    Serial.println("Failed to connect to WiFi. Restarting...");
    delay(5000);
    ESP.restart();
  }
  
  // Login to AlphaBase
  if (!loginAlphaBase()) {
    Serial.println("Failed to login to AlphaBase. Check credentials.");
    delay(5000);
    ESP.restart();
  }
  
  Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
  // Simulate reading a sensor (replace with real sensor)
  float temperature = random(200, 300) / 10.0;  // Random temp between 20-30°C
  float humidity = random(400, 700) / 10.0;     // Random humidity 40-70%
  
  Serial.println("--- Sending Sensor Data ---");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  // Create JSON data
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();
  doc["device"] = "ESP32-01";
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  // Send to AlphaBase
  if (sendData("sensors", "esp32_01", jsonData)) {
    Serial.println("✓ Data sent successfully!");
  } else {
    Serial.println("✗ Failed to send data");
  }
  
  // Retrieve data example
  Serial.println("\n--- Retrieving Data ---");
  if (getData("sensors", "esp32_01")) {
    Serial.println("✓ Data retrieved successfully!");
  }
  
  Serial.println("\nWaiting 10 seconds...\n");
  delay(10000);  // Send data every 10 seconds
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

// Login to AlphaBase and get authentication token
bool loginAlphaBase() {
  Serial.println("Logging in to AlphaBase...");
  
  HTTPClient http;
  String url = String(alphabaseURL) + "/auth/login";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // Create login JSON
  StaticJsonDocument<200> doc;
  doc["username"] = alphabaseUsername;
  doc["password"] = alphabasePassword;
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  // Send POST request
  int httpCode = http.POST(jsonData);
  
  if (httpCode == 200) {
    String response = http.getString();
    
    // Parse response to get token
    StaticJsonDocument<512> responseDoc;
    deserializeJson(responseDoc, response);
    
    authToken = responseDoc["access_token"].as<String>();
    
    Serial.println("✓ Login successful!");
    Serial.print("Token: ");
    Serial.println(authToken.substring(0, 20) + "...");
    
    http.end();
    return true;
  }
  
  Serial.print("✗ Login failed. HTTP Code: ");
  Serial.println(httpCode);
  http.end();
  return false;
}

// Send data to AlphaBase
bool sendData(String collection, String key, String jsonValue) {
  if (authToken == "") {
    Serial.println("✗ Not authenticated. Login first.");
    return false;
  }
  
  HTTPClient http;
  String url = String(alphabaseURL) + "/data/set";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);
  
  // Create data JSON
  StaticJsonDocument<512> doc;
  doc["collection"] = collection;
  doc["key"] = key;
  
  // Parse the value JSON
  StaticJsonDocument<512> valueDoc;
  deserializeJson(valueDoc, jsonValue);
  doc["value"] = valueDoc;
  
  String requestData;
  serializeJson(doc, requestData);
  
  // Send POST request
  int httpCode = http.POST(requestData);
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    return true;
  }
  
  Serial.print("✗ Failed to send data. HTTP Code: ");
  Serial.println(httpCode);
  Serial.println(http.getString());
  http.end();
  return false;
}

// Get data from AlphaBase
bool getData(String collection, String key) {
  if (authToken == "") {
    Serial.println("✗ Not authenticated. Login first.");
    return false;
  }
  
  HTTPClient http;
  String url = String(alphabaseURL) + "/data/get/" + collection + "/" + key;
  
  http.begin(url);
  http.addHeader("Authorization", "Bearer " + authToken);
  
  // Send GET request
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    
    // Parse and display response
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, response);
    
    Serial.println("Retrieved data:");
    serializeJsonPretty(doc, Serial);
    Serial.println();
    
    http.end();
    return true;
  }
  
  Serial.print("✗ Failed to get data. HTTP Code: ");
  Serial.println(httpCode);
  http.end();
  return false;
}