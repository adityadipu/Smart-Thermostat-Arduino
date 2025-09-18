/*
 * =================================================================
 * FINAL PROJECT: Climate Control with Battery Monitor
 * (V3.2: Interactive OLED Dashboard)
 * =================================================================
 */

// --- Include Libraries ---
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Pin Configuration ---
#define ONE_WIRE_BUS 2
const int redPin = 5, bluePin = 6, buzzerPin = 3, fanPin = 7, batteryPin = A0;

// --- Battery Monitor Configuration ---
const float R1 = 10000.0, R2 = 10000.0, LOW_BATT_VOLTAGE = 7.0;

// --- Temperature Thresholds ---
const float MAX_TEMP_C = 35.0, MIN_TEMP_C = 20.0;

// --- Alarm Customization ---
const int HIGH_TONE = 1000, LOW_TONE = 750, TONE_DURATION_MS = 250;
const int LIGHT_FLASH_MS = 80, LIGHT_PAUSE_MS = 50, LIGHT_LONG_PAUSE_MS = 200;

// --- OLED Display & Sensor Setup ---
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {
  // Pin Modes
  pinMode(redPin, OUTPUT); pinMode(bluePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT); pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, HIGH);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.clearDisplay(); display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 25); display.println("CLIMATE CTRL"); display.display();
  delay(2000);

  // Initialize Temperature Sensor
  sensors.begin();
}

void loop() {
  // 1. Get Sensor Readings
  sensors.requestTemperatures();
  float currentTempC = sensors.getTempCByIndex(0);
  float batteryVoltage = getBatteryVoltage();
  
  // 2. Main Logic
  String statusMessage = "SAFE";
  String fanStatus = "OFF";
  bool alarmActive = false;

  if (currentTempC < -50) {
    statusMessage = "SENSOR ERR";
    digitalWrite(fanPin, HIGH);
  } else if (currentTempC > MAX_TEMP_C) {
    statusMessage = "TOO HOT!";
    fanStatus = "ON";
    alarmActive = true;
    digitalWrite(fanPin, LOW);
  } else if (currentTempC < MIN_TEMP_C) {
    statusMessage = "TOO COLD!";
    alarmActive = true;
    digitalWrite(fanPin, HIGH);
  } else {
    digitalWrite(fanPin, HIGH);
  }

  // 3. Update OLED Display
  updateOLED(currentTempC, batteryVoltage, statusMessage, fanStatus);

  // 4. Control the Alarm
  if (alarmActive) { runAlarm(); } else { stopAlarm(); }
  
  delay(200); // Shortened delay for smoother fan animation
}

float getBatteryVoltage() {
  int sensorVal = analogRead(batteryPin);
  float voltageAtPin = sensorVal * (5.0 / 1023.0);
  float sourceVoltage = voltageAtPin * ((R1 + R2) / R2);
  return sourceVoltage;
}

// =================================================================
// --- NEW & IMPROVED updateOLED() Function ---
// =================================================================
void updateOLED(float temp, float batt, String status, String fanStat) {
  display.clearDisplay();

  // --- 1. Draw Top Status Bar ---
  display.setTextSize(1);
  
  // Draw Animated Fan Icon
  display.setCursor(0, 0);
  if (fanStat == "ON") {
    static int fanFrame = 0;
    const char* fanFrames = "|/-\\";
    display.print("[");
    display.print(fanFrames[fanFrame]);
    display.print("]");
    fanFrame = (fanFrame + 1) % 4; // Cycle through 4 animation frames
  } else {
    display.print("[*]"); // Static 'off' icon
  }

  // Draw Battery Icon and Text
  display.setCursor(30, 0);
  if (batt < LOW_BATT_VOLTAGE) {
    display.print("[!] LOW BATT!");
  } else if (batt < 7.5) {
    display.print("[|    ]");
  } else if (batt < 8.2) {
    display.print("[||   ]");
  } else {
    display.print("[|||  ]");
  }
  display.print(" ");
  display.print(batt, 1);
  display.print("V");

  // --- 2. Draw Main Temperature Display ---
  display.setTextSize(3);
  // Center the text horizontally
  int16_t x1, y1;
  uint16_t w, h;
  String tempStr = String(temp, 1);
  display.getTextBounds(tempStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 20);
  
  display.print(tempStr);
  display.setTextSize(2);
  display.cp437(true);
  display.write(248); // Degree symbol
  display.print("C");
  
  // --- 3. Draw Bottom Status Message ---
  display.setTextSize(2);
  display.getTextBounds(status, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 50);
  
  // Make alarm text blink
  if (status != "SAFE") {
    // millis() % 1000 gives a value that repeats every second
    // We only draw the text for the first half of the second
    if (millis() % 1000 < 500) {
      display.println(status);
    }
  } else {
    display.println(status);
  }

  display.display();
}

void runAlarm() {
  unsigned long currentTime = millis();
  static unsigned long lastSirenToggleTime = 0;
  static bool isHighTone = true;
  if (currentTime - lastSirenToggleTime >= TONE_DURATION_MS) {
    lastSirenToggleTime = currentTime;
    isHighTone = !isHighTone;
    if (isHighTone) { tone(buzzerPin, HIGH_TONE); } else { tone(buzzerPin, LOW_TONE); }
  }
  static unsigned long lastLightChangeTime = 0;
  static int lightState = 0;
  unsigned long interval = LIGHT_FLASH_MS;
  if (lightState == 1 || lightState == 5) interval = LIGHT_PAUSE_MS;
  if (lightState == 3 || lightState == 7) interval = LIGHT_LONG_PAUSE_MS;
  if (currentTime - lastLightChangeTime > interval) {
    lastLightChangeTime = currentTime;
    lightState = (lightState + 1) % 8;
    switch(lightState) {
      case 0: case 2: digitalWrite(redPin, HIGH); digitalWrite(bluePin, LOW); break;
      case 4: case 6: digitalWrite(redPin, LOW); digitalWrite(bluePin, HIGH); break;
      default: digitalWrite(redPin, LOW); digitalWrite(bluePin, LOW); break;
    }
  }
}

void stopAlarm() {
  noTone(buzzerPin);
  digitalWrite(redPin, LOW);
  digitalWrite(bluePin, LOW);
}