#include "CxESPConsoleExt.hpp"

CxESPConsoleExt ESPConsole(Serial, "Test App", "1.0");

WiFiServer server(23);

char ssid[20];
char password[25];

void setup() {
   Serial.begin(115200);
   
   //
   // Get the wifi ssid and password from the console settings.
   // The ssid and password can be set in the console with the commands
   //   wifi ssid <ssid>
   //   wifi pass <password>
   // These credentials will be stored in the EEPROM.
   //
   
   ESPConsole.getSSID(ssid, sizeof(ssid));
   ESPConsole.getPassword(password, sizeof(password));
   
   WiFi.begin(ssid, password);

   // try to connect to the network for max. 10 seconds
   while (WiFi.status() != WL_CONNECTED && millis() < 10000) {
      delay(500);
      Serial.print(".");
   }
   
   if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nWiFi not connected.");
   } else {
      Serial.println("\nWiFi connected.");
   }
   
   server.begin();
   
   ESPConsole.begin(server);
}

void loop() {
   ESPConsole.loop();
}
