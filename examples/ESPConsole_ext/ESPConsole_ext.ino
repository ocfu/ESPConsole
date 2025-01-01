#include "CxESPConsoleExt.hpp"

CxESPConsoleExt ESPConsole(Serial, "Test App", "1.0");

void setup() {
   Serial.begin(115200);
   ESPConsole.begin();
}

void loop() {
   ESPConsole.loop();
}
