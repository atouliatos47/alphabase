
/*
 * Power Press Simulator + AlphaBase Integration
 * 
 * Features:
 * - 3 Power Presses with state control
 * - HTTP REST API for data logging to AlphaBase
 * - MQTT for real-time monitoring
 * - Web server for local status
 * - Full integration with AlphaBase backend
 */

// Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

// WiFi Credentials
const char* ssid = "SKYPL2JH";
const char* password = "zNeUN3iQa2AbCJ";

// AlphaBase Configuration
const char* alphabaseURL = "http://192.168.0.52:8000";
const char* alphabaseUsername = "atoul";
const char* alphabasePassword = "password123";
String authToken = "";

// MQTT Configuration
const char* mqttServer = "192.168.0.52";
const int mqttPort = 1883;
const char* mqttTopicStatus = "alphabase/presses/status";
const char* mqttTopicCommands = "alphabase/presses/commands";

// Device Info
const char* deviceID = "Press-Simulator-01";

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Press 1
const int PIN_BUTTON_1 = 15;
const int PIN_RED_LED_1 = 2;
const int PIN_GREEN_LED_1 = 4;

// Press 2
const int PIN_BUTTON_2 = 5;
const int PIN_RED_LED_2 = 18;
const int PIN_GREEN_LED_2 = 19;

// Press 3
const int PIN_BUTTON_3 = 21;
const int PIN_RED_LED_3 = 22;
const int PIN_GREEN_LED_3 = 23;

// ============================================================================
// STATE MACHINE
// ============================================================================

enum PressState {
  IDLE,
  RUNNING
};

PressState press1State = IDLE;
PressState press2State = IDLE;
PressState press3State = IDLE;

// Track state changes for logging
bool press1Changed = false;
bool press2Changed = false;
bool press3Changed = false;

// ============================================================================
// CLIENTS
// ============================================================================

WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============================================================================
// BUTTON DEBOUNCING
// ============================================================================

int button1State = HIGH;
int lastButton1State = HIGH;
unsigned long lastDebounceTime1 = 0;

int button2State = HIGH;
int lastButton2State = HIGH;
unsigned long lastDebounceTime2 = 0;

int button3State = HIGH;
int lastButton3State = HIGH;
unsigned long lastDebounceTime3 = 0;

unsigned long debounceDelay = 50;

// ============================================================================
// LED BLINKING
// ============================================================================

unsigned long lastBlinkTime1 = 0;
unsigned long lastBlinkTime2 = 0;
unsigned long lastBlinkTime3 = 0;
const long blinkInterval = 1000;

bool greenLedState1 = LOW;
bool greenLedState2 = LOW;
bool greenLedState3 = LOW;

// ============================================================================
// TIMING
// ============================================================================

unsigned long lastMQTTPublish = 0;
const long mqttPublishInterval = 5000;  // Publish every 5 seconds

unsigned long lastHTTPLog = 0;
const long httpLogInterval = 30000;  // Log to AlphaBase every 30 seconds

// ============================================================================
// WIFI CONNECTION
// ============================================================================

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ WiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// ============================================================================
// ALPHABASE AUTHENTICATION
// ============================================================================

bool loginAlphaBase() {
  Serial.println("Logging in to AlphaBase...");
  
  HTTPClient http;
  String url = String(alphabaseURL) + "/auth/login";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  StaticJsonDocument<200> doc;
  doc["username"] = alphabaseUsername;
  doc["password"] = alphabasePassword;
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  int httpCode = http.POST(jsonData);
  
  if (httpCode == 200) {
    String response = http.getString();
    StaticJsonDocument<512> responseDoc;
    deserializeJson(responseDoc, response);
    
    authToken = responseDoc["access_token"].as<String>();
    Serial.println("✅ AlphaBase Login Successful!");
    http.end();
    return true;
  }
  
  Serial.print("❌ AlphaBase Login Failed. HTTP Code: ");
  Serial.println(httpCode);
  http.end();
  return false;
}

// ============================================================================
// ALPHABASE DATA LOGGING (HTTP)
// ============================================================================

void logStateToAlphaBase(int pressNumber, String state) {
  if (authToken == "") {
    Serial.println("⚠️  Not authenticated. Skipping log.");
    return;
  }
  
  HTTPClient http;
  String url = String(alphabaseURL) + "/data/set";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);
  
  StaticJsonDocument<512> doc;
  doc["collection"] = "presses";
  doc["key"] = String("press") + String(pressNumber) + "_" + String(millis());
  
  StaticJsonDocument<256> valueDoc;
  valueDoc["press_number"] = pressNumber;
  valueDoc["state"] = state;
  valueDoc["timestamp"] = millis();
  valueDoc["device_id"] = deviceID;
  
  doc["value"] = valueDoc;
  
  String requestData;
  serializeJson(doc, requestData);
  
  int httpCode = http.POST(requestData);
  
  if (httpCode == 200) {
    Serial.print("✅ Press ");
    Serial.print(pressNumber);
    Serial.print(" state logged: ");
    Serial.println(state);
  } else {
    Serial.print("❌ Failed to log. HTTP Code: ");
    Serial.println(httpCode);
  }
  
  http.end();
}

// ============================================================================
// MQTT CONNECTION
// ============================================================================

void connectMQTT() {
  Serial.print("Connecting to MQTT");
  
  while (!mqttClient.connected()) {
    Serial.print(".");
    
    String clientId = "PressSimulator-" + String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("\n✅ MQTT Connected!");
      mqttClient.subscribe(mqttTopicCommands);
      Serial.print("Subscribed to: ");
      Serial.println(mqttTopicCommands);
      return;
    }
    
    Serial.print("❌ MQTT Failed. State: ");
    Serial.println(mqttClient.state());
    delay(2000);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n📨 MQTT Command Received:");
  Serial.print("Topic: ");
  Serial.println(topic);
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message: ");
  Serial.println(message);
  
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("❌ JSON parsing failed");
    return;
  }
  
  const char* command = doc["command"];
  int pressNumber = doc["press"];
  
  // Handle remote commands
  if (strcmp(command, "start") == 0) {
    if (pressNumber == 1 && press1State == IDLE) {
      press1State = RUNNING;
      press1Changed = true;
      Serial.println("🟢 Press 1 Started Remotely");
    } else if (pressNumber == 2 && press2State == IDLE) {
      press2State = RUNNING;
      press2Changed = true;
      Serial.println("🟢 Press 2 Started Remotely");
    } else if (pressNumber == 3 && press3State == IDLE) {
      press3State = RUNNING;
      press3Changed = true;
      Serial.println("🟢 Press 3 Started Remotely");
    }
  } else if (strcmp(command, "stop") == 0) {
    if (pressNumber == 1 && press1State == RUNNING) {
      press1State = IDLE;
      press1Changed = true;
      Serial.println("🔴 Press 1 Stopped Remotely");
    } else if (pressNumber == 2 && press2State == RUNNING) {
      press2State = IDLE;
      press2Changed = true;
      Serial.println("🔴 Press 2 Stopped Remotely");
    } else if (pressNumber == 3 && press3State == RUNNING) {
      press3State = IDLE;
      press3Changed = true;
      Serial.println("🔴 Press 3 Stopped Remotely");
    }
  }
}

// ============================================================================
// MQTT PUBLISH STATUS
// ============================================================================

void publishStatusMQTT() {
  StaticJsonDocument<256> doc;
  doc["device_id"] = deviceID;
  doc["press1"] = (press1State == RUNNING) ? "RUNNING" : "IDLE";
  doc["press2"] = (press2State == RUNNING) ? "RUNNING" : "IDLE";
  doc["press3"] = (press3State == RUNNING) ? "RUNNING" : "IDLE";
  doc["timestamp"] = millis();
  doc["ip"] = WiFi.localIP().toString();
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  if (mqttClient.publish(mqttTopicStatus, jsonData.c_str())) {
    Serial.println("📡 Status published to MQTT");
  }
}

// ============================================================================
// WEB SERVER HANDLER
// ============================================================================

void handleGetStatus() {
  String json = "{";
  json += "\"device_id\": \"" + String(deviceID) + "\",";
  json += "\"press1\": \"" + String((press1State == RUNNING) ? "RUNNING" : "IDLE") + "\",";
  json += "\"press2\": \"" + String((press2State == RUNNING) ? "RUNNING" : "IDLE") + "\",";
  json += "\"press3\": \"" + String((press3State == RUNNING) ? "RUNNING" : "IDLE") + "\",";
  json += "\"uptime\": " + String(millis() / 1000);
  json += "}";
  
  server.send(200, "application/json", json);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  Power Press + AlphaBase Integration  ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Connect WiFi
  connectToWiFi();
  
  // Login to AlphaBase
  if (!loginAlphaBase()) {
    Serial.println("⚠️  Continuing without AlphaBase...");
  }
  
  // Setup MQTT
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  connectMQTT();
  
  // Start Web Server
  server.on("/status", HTTP_GET, handleGetStatus);
  server.begin();
  Serial.println("🌐 Web Server Started on /status");
  
  // Initialize Pins
  pinMode(PIN_RED_LED_1, OUTPUT);
  pinMode(PIN_GREEN_LED_1, OUTPUT);
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  
  pinMode(PIN_RED_LED_2, OUTPUT);
  pinMode(PIN_GREEN_LED_2, OUTPUT);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  
  pinMode(PIN_RED_LED_3, OUTPUT);
  pinMode(PIN_GREEN_LED_3, OUTPUT);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  
  // Set Initial State
  digitalWrite(PIN_RED_LED_1, HIGH);
  digitalWrite(PIN_GREEN_LED_1, LOW);
  digitalWrite(PIN_RED_LED_2, HIGH);
  digitalWrite(PIN_GREEN_LED_2, LOW);
  digitalWrite(PIN_RED_LED_3, HIGH);
  digitalWrite(PIN_GREEN_LED_3, LOW);
  
  Serial.println("\n✅ Setup Complete!\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  // Maintain WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected. Reconnecting...");
    connectToWiFi();
  }
  
  // Maintain MQTT
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
  
  // Handle Web Server
  server.handleClient();
  
  // Handle Press Controls
  handleButton1();
  handleButton2();
  handleButton3();
  
  updatePress1LEDs();
  updatePress2LEDs();
  updatePress3LEDs();
  
  // Log state changes to AlphaBase (HTTP)
  if (press1Changed) {
    logStateToAlphaBase(1, (press1State == RUNNING) ? "RUNNING" : "IDLE");
    press1Changed = false;
  }
  if (press2Changed) {
    logStateToAlphaBase(2, (press2State == RUNNING) ? "RUNNING" : "IDLE");
    press2Changed = false;
  }
  if (press3Changed) {
    logStateToAlphaBase(3, (press3State == RUNNING) ? "RUNNING" : "IDLE");
    press3Changed = false;
  }
  
  // Periodic MQTT status publish
  unsigned long currentMillis = millis();
  if (currentMillis - lastMQTTPublish >= mqttPublishInterval) {
    lastMQTTPublish = currentMillis;
    publishStatusMQTT();
  }
}

// ============================================================================
// PRESS 1 FUNCTIONS
// ============================================================================

void handleButton1() {
  int reading = digitalRead(PIN_BUTTON_1);
  
  if (reading != lastButton1State) {
    lastDebounceTime1 = millis();
  }
  
  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading != button1State) {
      button1State = reading;
      if (button1State == LOW) {
        Serial.println("🔘 Button 1 Pressed!");
        if (press1State == IDLE) {
          press1State = RUNNING;
          lastBlinkTime1 = millis();
          greenLedState1 = HIGH;
        } else {
          press1State = IDLE;
        }
        press1Changed = true;
      }
    }
  }
  lastButton1State = reading;
}

void updatePress1LEDs() {
  switch (press1State) {
    case IDLE:
      digitalWrite(PIN_RED_LED_1, HIGH);
      digitalWrite(PIN_GREEN_LED_1, LOW);
      break;
    case RUNNING:
      digitalWrite(PIN_RED_LED_1, LOW);
      if (millis() - lastBlinkTime1 >= blinkInterval) {
        lastBlinkTime1 = millis();
        greenLedState1 = !greenLedState1;
        digitalWrite(PIN_GREEN_LED_1, greenLedState1);
      }
      break;
  }
}

// ============================================================================
// PRESS 2 FUNCTIONS
// ============================================================================

void handleButton2() {
  int reading = digitalRead(PIN_BUTTON_2);
  
  if (reading != lastButton2State) {
    lastDebounceTime2 = millis();
  }
  
  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading != button2State) {
      button2State = reading;
      if (button2State == LOW) {
        Serial.println("🔘 Button 2 Pressed!");
        if (press2State == IDLE) {
          press2State = RUNNING;
          lastBlinkTime2 = millis();
          greenLedState2 = HIGH;
        } else {
          press2State = IDLE;
        }
        press2Changed = true;
      }
    }
  }
  lastButton2State = reading;
}

void updatePress2LEDs() {
  switch (press2State) {
    case IDLE:
      digitalWrite(PIN_RED_LED_2, HIGH);
      digitalWrite(PIN_GREEN_LED_2, LOW);
      break;
    case RUNNING:
      digitalWrite(PIN_RED_LED_2, LOW);
      if (millis() - lastBlinkTime2 >= blinkInterval) {
        lastBlinkTime2 = millis();
        greenLedState2 = !greenLedState2;
        digitalWrite(PIN_GREEN_LED_2, greenLedState2);
      }
      break;
  }
}

// ============================================================================
// PRESS 3 FUNCTIONS
// ============================================================================

void handleButton3() {
  int reading = digitalRead(PIN_BUTTON_3);
  
  if (reading != lastButton3State) {
    lastDebounceTime3 = millis();
  }
  
  if ((millis() - lastDebounceTime3) > debounceDelay) {
    if (reading != button3State) {
      button3State = reading;
      if (button3State == LOW) {
        Serial.println("🔘 Button 3 Pressed!");
        if (press3State == IDLE) {
          press3State = RUNNING;
          lastBlinkTime3 = millis();
          greenLedState3 = HIGH;
        } else {
          press3State = IDLE;
        }
        press3Changed = true;
      }
    }
  }
  lastButton3State = reading;
}

void updatePress3LEDs() {
  switch (press3State) {
    case IDLE:
      digitalWrite(PIN_RED_LED_3, HIGH);
      digitalWrite(PIN_GREEN_LED_3, LOW);
      break;
    case RUNNING:
      digitalWrite(PIN_RED_LED_3, LOW);
      if (millis() - lastBlinkTime3 >= blinkInterval) {
        lastBlinkTime3 = millis();
        greenLedState3 = !greenLedState3;
        digitalWrite(PIN_GREEN_LED_3, greenLedState3);
      }
      break;
  }
}