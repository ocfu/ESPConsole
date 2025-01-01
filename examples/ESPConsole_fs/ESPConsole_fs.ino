#include "CxESPConsoleFS.hpp"

CxESPConsoleFS ESPConsole(Serial, "Test App", "1.0");

void setup() {
   Serial.begin(115200);
   ESPConsole.begin();
}

void loop() {
   ESPConsole.loop();
}

