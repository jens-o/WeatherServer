/*
  WeatherServer
  
  Written in 2017 by Jens Olsson.
*/

// Import required libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>

#include "wifiparams.h"

// Data wire is plugged into port on the Arduino
#define ONE_WIRE_BUS 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

// Create Serial for the WiFi card
SoftwareSerial Serial1(6, 7);

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

int status = WL_IDLE_STATUS;

// Create an instance of the server
WiFiEspServer server(LISTEN_PORT);

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  Serial1.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  //Get devices
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
  
  // Initiate WiFi
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");

    while(true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attemting to connect to SSID: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWiFiStatus();

  // Start the server
  server.begin();
}

void loop() {

  // Handle REST calls
  WiFiEspClient client = server.available();

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        process(client);
      }
  
      delay(1);
  
      client.stop();

      while(client.status() != 0) {
        delay(5);
      }
    }
  }
}

void process(WiFiEspClient client) {

  // Jump over HTTP action
  client.readStringUntil('/');

  // Get the command
  String command = client.readStringUntil(' ');

  if (command == "weather") {
    temperature(client);
  }
  else {
    Serial.print("Received unknown command: ");
    Serial.println(command);

    client.print(
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/html\r\n"
            "\r\n");
  }
}

void temperature(WiFiEspClient client)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  // method 2 - faster
  float tempC = sensors.getTempC(insideThermometer);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit

  client.flush();

  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/json\r\n"
            "Connection: close\r\n"
            "\r\n");
            
  client.print("{\r\n");
  client.print("\"temperature\": ");
  client.print(tempC);
  client.print(",\r\n");
  client.print("\"humidity\": ");
  client.print(0);
  client.print("\r\n");
  client.print("}\r\n");
  client.print("\r\n");

  /*
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
  */

  
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("AP Address: ");
  Serial.println(ip);
}
