
#include <ESP8266WiFi.h>
#include <aREST.h>
// Needs to be from https://github.com/mkokotovich/aREST_UI until pull request is merged in
#include <aREST_UI.h>
#include "private.h"

const char* ssid     = private_ssid;
const char* password = private_password;

#define READ_TIMEOUT 10000 //10 seconds
#define NUM_GREEN 4
#define NUM_YELLOW 4

// Create LED arrays
int green_leds[NUM_GREEN] = {D1, D2, D3, D4};
int yellow_leds[NUM_YELLOW] = {D5, D6, D7, D8};

// Create aREST instance
aREST_UI rest = aREST_UI();

// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Variable to hold the current state, which will be displayed
String current_state = "UNINITIALIZED";

// Variable to hold the timer state, which will be displayed
String timer_state = "Disabled";

unsigned long timer_state_last_updated = 0;
unsigned long timer_ms_start = 0;
unsigned long timer_ms_duration = 0;

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
int clockOff(String command) {
  controlGreen(LOW);
  controlYellow(LOW);
  current_state = "Off";
  return 1;
}
int clockYellow(String command) {
  controlYellow(HIGH);
  controlGreen(LOW);
  current_state = "Yellow";
  return 1;
}
int clockGreen(String command) {
  controlYellow(LOW);
  controlGreen(HIGH);
  current_state = "Green";
  return 1;
}
int clockTimer(String command) {
  Serial.print("clockTimer called with: ");
  Serial.println(command);

  clockYellow("");
  
  timer_ms_start = millis();
  // Passed to us in minutes, need to convert
  timer_ms_duration = strtoul(command.c_str(), NULL, 10) * 60 * 1000;
  timer_state_last_updated = 0;
  timer_state = String("Enabled, ") + command + String(" minutes left");
  return 1;
}
int cancelTimer(String command) {
  timer_state_last_updated = 0;
  timer_ms_start = 0;
  timer_ms_duration = 0;
  timer_state="Disabled";
  return 1;
}

void startWifi(void)
{
  // Connect to WiFi
  WiFi.disconnect(); //no-op if not connected
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void startWifiServer(void)
{
  // Start the server
  server.begin();
  Serial.println("Server started");
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

  // Display the current state
  rest.variable_label("current_state", "Current state of clock", &current_state);
  
  // Register the functions with the rest UI
  
  rest.function_button("clockOff", "Off", clockOff);
  rest.function_button("clockYellow", "Yellow", clockYellow);
  rest.function_button("clockGreen", "Green", clockGreen);
  
  rest.label("Turn on sleep and set to wake in:");
  rest.function_with_input_button("clockTimer", "minutes", clockTimer);
  // Display the timer state
  rest.variable_label("timer_state", "Timer", &timer_state);
  rest.function_button("cancelTimer", "Cancel Timer", cancelTimer);
  
  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("esp8266");
  rest.title("Ryan's Clock");

  clockOff("");
  
  startWifi();
  
  startWifiServer();
  
  // Print the IP address
  Serial.println(WiFi.localIP());
  
}

void loop() {
  // Reset device if wifi is disconnected
  if (WiFi.status() == WL_DISCONNECTED)
  {
    Serial.println("Wifi diconnected, reset connection");
    
    startWifi();
    startWifiServer();
  }

  // Handle timer
  if (timer_ms_start != 0)
  {
    unsigned long ms_elapsed = 0;
    if ((ms_elapsed = (millis() - timer_ms_start)) > timer_ms_duration)
    {
      Serial.println("Timer is up!");
      clockGreen("");
      cancelTimer("");
    }
    else if (ms_elapsed > 1000 + timer_state_last_updated)
    {
      timer_state_last_updated = ms_elapsed;
      timer_state = String("Enabled, ") + ceil(((float)(timer_ms_duration - ms_elapsed))/1000/60) + String(" minutes left");
    }
  }
  
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }
  delay(50); //devices seems to choke if we read too fast, even with below wait
  unsigned long startMillis = millis();
  while (!client.available() && (millis() - startMillis) < READ_TIMEOUT)
  {
      yield();
  }
  if (!client.available())
  {
    Serial.println("Read timeout reached, dropping client");
    client.stop();
    return;
  }
  
  rest.handle(client);
 
}

