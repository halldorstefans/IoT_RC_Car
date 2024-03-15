#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>

#include "arduino_secrets.h" 
/////// Please enter your sensitive data in the Secret tab/arduino_secrets.h
const char ssid[]       = SECRET_SSID;    // your network SSID (name)
const char pass[]       = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
const char broker[]     = SECRET_BROKER;
const char* certificate = SECRET_CERTIFICATE;

int status = WL_IDLE_STATUS;

WiFiServer server(80);

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

const char *webpage = 
#include "controlPage.h"
;

//const int capacity = JSON_OBJECT_SIZE(2);
//StaticJsonDocument<capacity> doc;
JsonDocument doc;
char jsonOutput[128];

void setup() {  
  Serial.begin(115200);
  //while (!Serial);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);
  
  pinMode(SS, OUTPUT); 
  digitalWrite(SS, HIGH);

  // Put SCK, MOSI, SS pins into output mode
  // also put SCK, MOSI into LOW state, and SS into HIGH state.
  // Then put SPI hardware into Master mode and turn SPI on
  SPI.begin();

  // Slow down the master a bit
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  enable_WiFi();

  connect_WiFi();

  server.begin();
  // you're connected now, so print out the status:
  printWifiStatus();

  // turn on the LED once we've connected to the Wi-Fi
  digitalWrite(LED_BUILTIN, HIGH);
  connect_MQTT();
}

void loop() {
  // listen for incoming clients
  WiFiClient wifiClient = server.available();
  
  if (wifiClient) {
    if (WiFi.status() != WL_CONNECTED) {
      connect_WiFi();
    }
  
    if (!mqttClient.connected()) {
      // MQTT client is disconnected, connect
      connect_MQTT();
    }
    
    Serial.println("new client");
    byte dist_msg;
    String currentLine = "";
    // an HTTP request ends with a blank line
    boolean currentLineIsBlank = true;
    while (wifiClient.connected()) {
      digitalWrite(LED_BUILTIN, HIGH);
      if (wifiClient.available()) {
        char c = wifiClient.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply        
        if (c == '\n' && currentLine.length() == 0) {
          // send a standard HTTP response header
          server.print(webpage);
          break;
        }
        
        if (c == '\n') {
          // you're starting a new line
          currentLine = "";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLine += c;
          if (currentLine.endsWith("GET /left")) {
            dist_msg = transferAndWait('v');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /forward")) {
            dist_msg = transferAndWait('f');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /right")) {
            dist_msg = transferAndWait('h');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /reverse")) {
            dist_msg = transferAndWait('r');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /straight")) {
            dist_msg = transferAndWait('b');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /stop")) {
            dist_msg = transferAndWait('x');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /play")) {
            dist_msg = transferAndWait('p');
            publishMessage(dist_msg, getDirection(currentLine));
          }
          if (currentLine.endsWith("GET /pause")) {
            dist_msg = transferAndWait('s');
            publishMessage(dist_msg, getDirection(currentLine));
          }
        }
      }
    }

    // close the connection:
    wifiClient.stop();
    Serial.println("client disconnected");
    // turn off the LED once we've disconnected to the Wi-Fi
    digitalWrite(LED_BUILTIN, LOW);
  }
}

/*** Methods ***/

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

byte transferAndWait(const byte what)
{
  digitalWrite(SS, LOW);
  byte a = SPI.transfer(what);
  digitalWrite(SS, HIGH);
  delayMicroseconds(20);
  return a;
} // end of transferAndWait

String getDirection(String requestString) {
  int getTag = requestString.indexOf("GET /");
  return requestString.substring(getTag+5);
}

void enable_WiFi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
}

void connect_WiFi() {
  // attempt to connect to WiFi network:
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  Serial.print(" ");
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void connect_MQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void publishMessage(byte msg, String dir) {
  Serial.println("Publishing message");
  Serial.println("Dir:");
  Serial.println(dir);

  doc["distance_cm"] = msg;
  doc["direction"] = dir;
  serializeJson(doc, jsonOutput);
  
  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/15/outgoing");
  mqttClient.print(jsonOutput);
  mqttClient.endMessage();
}
