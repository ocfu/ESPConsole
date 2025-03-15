///
/// ESPConsole - Bare minimum + WiFi
///
///
#include "CxESPConsole.hpp"
#include "../capabilities/CxCapabilityBasic.hpp"


WiFiServer server(8266);

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
   g_Stack.begin();

   Serial.begin(115200);
   Serial.println();
   
   WiFi.begin(STASSID, STAPSK);
   
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }
   Serial.println("\nWiFi connected.");
   
   ESPConsole.setAppNameVer("Min WiFi", "v1.0");
   ESPConsole.setTimeZone("CET-1CEST,M3.5.0,M10.5.0/3");

   CAPREG(CxCapabilityBasic);
   CAPLOAD(CxCapabilityBasic);
   
   ESPConsole.begin(server);
}

void loop() {
   ESPConsole.loop();
}
