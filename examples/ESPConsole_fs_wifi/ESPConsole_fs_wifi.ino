#include "CxESPConsoleFS.hpp"

CxESPConsoleFS ESPConsole(Serial, "Test App", "1.0");

WiFiServer server(23);

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
   Serial.begin(115200);
   WiFi.begin(STASSID, STAPSK);
   
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }
   Serial.println("\nWiFi connected.");
   server.begin();
   
   ESPConsole.begin(server);
}

void loop() {
   ESPConsole.loop();
}
