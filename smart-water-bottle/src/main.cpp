// Full fixed sketch: OLED init + throttled Firebase reads + write control on button press

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Pins
#define ONE_WIRE_BUS 4
#define BTN_UP 18
#define BTN_DOWN 19
#define BTN_OK 23
#define HEATER_PIN 26
#define COOLER_PIN 27

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Sensor + WiFi
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// Firebase endpoints
String telemetryURL = "https://smart-bottle-iot-default-rtdb.asia-southeast1.firebasedatabase.app/bottle/telemetry.json";
String controlSetURL = "https://smart-bottle-iot-default-rtdb.asia-southeast1.firebasedatabase.app/bottle/control/setpoint.json";

// Simulation & control
bool simulationMode = true;
float simTemperature = 25.0;
float ambientTemp = 25.0;
float setpoint = 50.0;
int heaterDuty = 0;
int coolerDuty = 0;

// timing/debounce
unsigned long lastOkPress = 0;
const unsigned long okDebounceMs = 250;
unsigned long lastBtnTime = 0;
unsigned long lastFirebaseRead = 0;
const unsigned long FIREBASE_READ_INTERVAL_MS = 5000; // 5s

// --- Helpers: HTTP write/read functions ---

void writeControlSetpointToFirebase(float newSetpoint)
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  HTTPClient http;
  http.begin(controlSetURL);
  http.addHeader("Content-Type", "text/plain");
  String body = String(newSetpoint, 2); // plain number
  int code = http.PUT(body);
  Serial.print("Write control/setpoint -> ");
  Serial.println(code);
  http.end();
}

float readSetpointFromFirebaseOnce()
{
  if (WiFi.status() != WL_CONNECTED)
    return setpoint;
  HTTPClient http;
  http.begin(controlSetURL);
  int code = http.GET();
  if (code == 200)
  {
    String payload = http.getString();
    // payload might be "30" or "30.0" possibly with quotes in some configs; toFloat handles it.
    float val = payload.toFloat();
    Serial.print("Read control/setpoint <- ");
    Serial.println(val);
    http.end();
    return val;
  }
  http.end();
  return setpoint;
}

void sendTelemetryToFirebase(float temp, float set, int heat, int cool, const String &mode)
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  HTTPClient http;
  http.begin(telemetryURL);
  http.addHeader("Content-Type", "application/json");
  String json = "{";
  json += "\"temperature\":" + String(temp, 2) + ",";
  json += "\"setpoint\":" + String(set, 2) + ",";
  json += "\"heater\":" + String(heat) + ",";
  json += "\"cooler\":" + String(cool) + ",";
  json += "\"mode\":\"" + mode + "\",";
  json += "\"ts\":" + String(millis() / 1000);
  json += "}";
  int code = http.PUT(json);
  Serial.print("Telemetry PUT -> ");
  Serial.println(code);
  http.end();
}

// --- Display / sensor helpers ---

void drawHeader()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Smart Bottle IoT");
  display.setCursor(0, 10);
  display.print("Mode: ");
  display.print(simulationMode ? "SIM" : "REAL");
  display.display();
}

// Read DS18B20 (returns NAN if disconnected)
float readRealSensor()
{
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C)
    return NAN;
  return t;
}

// --- Setup / Loop ---

void setup()
{
  Serial.begin(115200);
  // pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(COOLER_PIN, OUTPUT);

  // sensor + i2c
  sensors.begin();
  Wire.begin(21, 22);

  // initialize OLED robustly
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED Failed");
    // continue but OLED won't update — keep running so Firebase still works
  }
  else
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    // Quick visual test: show "OLED OK" then clear
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("OLED OK");
    display.display();
    delay(400);
    display.clearDisplay();
    display.display();
    Serial.println("OLED OK");
  }

  drawHeader();

  // WIFI
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected.");
  // Read initial setpoint from cloud once
  setpoint = readSetpointFromFirebaseOnce();
  // Write it back (ensure telemetry/control sync)
  writeControlSetpointToFirebase(setpoint);
}

void loop()
{
  // toggle mode (OK button) - short press
  if (digitalRead(BTN_OK) == LOW && millis() - lastOkPress > okDebounceMs)
  {
    lastOkPress = millis();
    simulationMode = !simulationMode;
    drawHeader();
    // publish mode change to telemetry immediately
    sendTelemetryToFirebase(simTemperature, setpoint, heaterDuty, coolerDuty, simulationMode ? "SIM" : "REAL");
  }

  // Buttons: update local setpoint AND push change to control/setpoint in Firebase
  if (millis() - lastBtnTime > 150)
  {
    if (digitalRead(BTN_UP) == LOW)
    {
      setpoint += 1.0;
      lastBtnTime = millis();
      Serial.print("Button UP -> setpoint=");
      Serial.println(setpoint);
      writeControlSetpointToFirebase(setpoint); // push to cloud
    }
    if (digitalRead(BTN_DOWN) == LOW)
    {
      setpoint -= 1.0;
      lastBtnTime = millis();
      Serial.print("Button DOWN -> setpoint=");
      Serial.println(setpoint);
      writeControlSetpointToFirebase(setpoint); // push to cloud
    }
  }

  // Periodically read cloud control (throttled)
  if (millis() - lastFirebaseRead > FIREBASE_READ_INTERVAL_MS)
  {
    float cloudSet = readSetpointFromFirebaseOnce();
    // only accept cloud setpoint if different (avoid unnecessary override if we just wrote it)
    if (abs(cloudSet - setpoint) > 0.001)
    {
      setpoint = cloudSet;
      Serial.print("Applied cloud setpoint -> ");
      Serial.println(setpoint);
    }
    lastFirebaseRead = millis();
  }

  // Temperature source selection
  float measuredTemp;
  if (simulationMode)
  {
    measuredTemp = simTemperature;
  }
  else
  {
    float realT = readRealSensor();
    if (!isnan(realT))
      measuredTemp = realT;
    else
      measuredTemp = simTemperature;
  }

  // controller (on/off)
  float error = setpoint - measuredTemp;
  if (error > 0.5)
  {
    heaterDuty = 1;
    coolerDuty = 0;
    digitalWrite(HEATER_PIN, HIGH);
    digitalWrite(COOLER_PIN, LOW);
  }
  else if (error < -0.5)
  {
    heaterDuty = 0;
    coolerDuty = 1;
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(COOLER_PIN, HIGH);
  }
  else
  {
    heaterDuty = 0;
    coolerDuty = 0;
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(COOLER_PIN, LOW);
  }

  // simulation physics
  if (simulationMode)
  {
    simTemperature += heaterDuty * 0.08;
    simTemperature -= coolerDuty * 0.08;
    simTemperature += (ambientTemp - simTemperature) * 0.01;
  }
  else
  {
    simTemperature = measuredTemp;
  }

  // Update OLED (if available)
  if (display.width() > 0)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("T:");
    display.print(measuredTemp, 1);
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print("S:");
    display.print(setpoint, 1);
    display.setCursor(80, 30);
    display.print(simulationMode ? "SIM" : "REAL");
    display.display();
  }

  // Serial telemetry
  Serial.print("{\"temp\":");
  Serial.print(measuredTemp, 2);
  Serial.print(",\"set\":");
  Serial.print(setpoint, 2);
  Serial.print(",\"heat\":");
  Serial.print(heaterDuty);
  Serial.print(",\"cool\":");
  Serial.print(coolerDuty);
  Serial.print(",\"mode\":\"");
  Serial.print(simulationMode ? "SIM" : "REAL");
  Serial.println("\"}");

  // Telemetry to Firebase (send every loop; cloud read is throttled separately)
  sendTelemetryToFirebase(measuredTemp, setpoint, heaterDuty, coolerDuty, simulationMode ? "SIM" : "REAL");

  delay(800); // small sleep
}