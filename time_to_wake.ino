#include <ESP8266WiFi.h>
#include "private.h"
#include "RestUI.h"
#include "OTAUpdates.h"
#include "AlarmHandler.h"
#include "ntptime.h"

#define ALARM_UPDATE_INTERVAL 2000

const char* ssid     = private_ssid;
const char* password = private_password;

#define READ_TIMEOUT 10000 //10 seconds
#define NUM_GREEN 4
#define NUM_YELLOW 3

// Create LED arrays
int green_leds[NUM_GREEN] = {D5, D6, D7, D8};
int yellow_leds[NUM_YELLOW] = {D1, D2, D3};

// Variable to hold the current state, which will be displayed
String current_state = "UNINITIALIZED";
// Variable to hold the timer state, which will be displayed
String activeAlarms = "None";

unsigned long lastAlarmUpdate = 0;

void controlGreen(int state)
{
  for (int i = 0; i < NUM_GREEN; i++)
  {
    digitalWrite(green_leds[i], state);
  }
}
void controlYellow(int state)
{
  for (int i = 0; i < NUM_YELLOW; i++)
  {
    digitalWrite(yellow_leds[i], state);
  }
}
void clockOff() {
  controlGreen(LOW);
  controlYellow(LOW);
  current_state = "Off";
}
void clockYellow() {
  controlYellow(HIGH);
  controlGreen(LOW);
  current_state = "Yellow";
}
void clockGreen() {
  controlYellow(LOW);
  controlGreen(HIGH);
  current_state = "Green";
}

void startWifi(void)
{
  // Connect to WiFi
  WiFi.disconnect(); //no-op if not connected
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("");
  Serial.println("Connecting to " + String(ssid));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup(void)
{  
  // Start Serial
  Serial.begin(115200);

  // Initialize all pins
  for (int i = 0; i < NUM_GREEN; i++)
  {
    pinMode(green_leds[i], OUTPUT);
  }
  for (int i = 0; i < NUM_YELLOW; i++)
  {
    pinMode(yellow_leds[i], OUTPUT);
  }

  clockOff();

  SPIFFS.begin();

  setupRestUI();
  
  startWifi();
  startRestUIServer();
  startUpdateServer();
  start_ntptime();
}

void loop() {
  // Reset device if wifi is disconnected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi diconnected, reset connection");
    
    startWifi();
    startRestUIServer();
    startUpdateServer();
    start_ntptime();
  }

  // Update active timers, if needed
  Alarm.delay(0);

  // Update activeAlarm label
  if (millis() - lastAlarmUpdate > ALARM_UPDATE_INTERVAL)
  {
    alarmHandler.updateActiveAlarms(activeAlarms);
    lastAlarmUpdate = millis();
  }
  
  handleOTAUpdate();
  handleRestUI();
}

