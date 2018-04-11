#define DEBUG_ESP_CORE 1

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP.h>

#include <BootstrapWebSite.h>
#include <BootstrapWebPage.h>

#include <IFTTTWebhook.h>

#include "config.h"

// hardware configuration
const int buttonPin = BUTTON_PIN;    // the number of the pushbutton pin
const int ledPin = LED_PIN;      // the number of the LED pin
const int relayPin = RELAY_PIN;

ESP8266WebServer server(80);
BootstrapWebSite ws("en", "Coffeebot");
IFTTTWebhook ifttt(IFTTT_API_KEY, IFTTT_EVENT_NAME);

int ledState = LOW;         // the current state of the output pin
int relayState = LOW;         // the current state of the output pin
unsigned long startTime;

volatile bool interrupted = false;

void handleInfo(), handleESP(), handleRoot(), handleNotFound(), handleButtonPress();

void handleInterrupt() {
  interrupted = true;
}

void setup() {
  startTime = millis();
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleInterrupt, FALLING);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(ledPin, LOW);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");
  Serial.println("attempting to connect");
  Serial.println("mac address");
  Serial.println(WiFi.macAddress());


  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("derp");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#ifdef MDNS_NAME
  if (MDNS.begin(MDNS_NAME)) {
    Serial.println("MDNS responder started");
  }
#endif

  ifttt.trigger("boot");

  ws.addBranding(BRANDING_IMAGE, BRANDING_IMAGE_TYPE);

  ws.addPageToNav(String("Info"), String("/info"));
  ws.addPageToNav(String("ESP"), String("/esp"));

  server.on("/", handleRoot);
  server.on("/info", handleInfo);
  server.on("/esp", handleESP);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void resetGroundsContainer() {
  Serial.println("closing grounds container switch...");
  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, HIGH);
  delay(GROUNDS_CONTAINER_SWITCH_DELAY);
  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW);
  Serial.println("... opening ground container switch");

  ifttt.trigger();
}

void loop() {
  server.handleClient();

  if (interrupted) {
    Serial.println("interrupto");

    resetGroundsContainer();
    interrupted = false;
  }
}

// we have implicit debounce because of the multi-second delay while we keep the switch closed

void handleInfo() {
  BootstrapWebPage page(&ws);

  Serial.println("got /info");

  page.addHeading(String("Info"));
  page.addList(String("Uptime ") + String((millis() - startTime) / 1000) + String(" seconds"),
         String("IP address ") + String(WiFi.localIP().toString()),
         String("Hostname ") + String(WiFi.hostname()),
         String("MAC address ") + WiFi.macAddress(),
         String("Subnet mask ") + WiFi.subnetMask().toString(),
         String("router IP ") + WiFi.gatewayIP().toString(),
         String("first DNS server ") + WiFi.dnsIP(0).toString(),
         String("SSID ") + WiFi.SSID(),
         String("RSSi ") + WiFi.RSSI());

  server.send(200, "text/html", page.getHTML());
}

void handleRoot() {
  Serial.println("got /");

  if (server.hasArg("button")) {
    handleButtonPress();
    return;
  }

  BootstrapWebPage page(&ws);
  page.addContent(String("<form class='form' method='get' action='/'><input type='hidden' name='button' value='1'><input type='submit' class='btn btn-success' value='Reset grounds container'></form>"));
  server.send(200, "text/html", page.getHTML());
}

void handleButtonPress() {
  Serial.println("form button presso");

  resetGroundsContainer();

  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}


void handleESP() {
  Serial.println("got /esp");

  BootstrapWebPage page(&ws);
  page.addHeading(String("ESP"));
  page.addList(String("VCC ") + ESP.getVcc(),
               String("Free heap ") + ESP.getFreeHeap(),
               String("Chip ID ") + ESP.getChipId() ,
               String("Flash chip ID ") + ESP.getFlashChipId(),
               String("Flash chip size ") + ESP.getFlashChipSize(),
               String("Flash chip speed ") + ESP.getFlashChipSpeed(),
               String("Sketch Size ") + ESP.getSketchSize(),
               String("Free Sketch Space ") + ESP.getFreeSketchSpace());

  server.send(200, "text/html", page.getHTML());
}

void handleNotFound() {
  Serial.println("got 404");

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

