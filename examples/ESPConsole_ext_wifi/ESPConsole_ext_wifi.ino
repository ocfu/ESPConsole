#include "CxESPConsoleExt.hpp"

CxESPConsoleExt ESPConsole(Serial, "Test App", "1.0");

WiFiServer server(23);

void setup() {
   Serial.begin(115200);
   
   //
   // Read the ssid, password and hostname from the console settings.
   // All can be set in the console with the commands
   //   wifi ssid <ssid>
   //   wifi password <password>
   //   wifi hostname <hostname>
   // These settings will be stored in the EEPROM.
   //
   
   String strSSID;
   String strPassword;
   String strHostName;
   
   ESPConsole.readSSID(strSSID);
   ESPConsole.readPassword(strPassword);
   ESPConsole.readHostName(strHostName);

   WiFi.begin(strSSID.c_str(), strPassword.c_str());
   WiFi.setAutoReconnect(true);
   WiFi.hostname(strHostName.c_str());

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
