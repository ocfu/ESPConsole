#include "CxESPConsoleMqtt.hpp"

CxESPConsoleMqtt ESPConsole(Serial, "My App", "1.0");

WiFiServer server(8266);  // for remote clients

void setup() {
   Serial.begin(115200);
   ESPConsole.begin(server);
}

void loop() {
   ESPConsole.loop();
}

