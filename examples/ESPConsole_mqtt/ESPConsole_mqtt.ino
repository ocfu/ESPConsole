#include "CxESPConsoleMqtt.hpp"

CxESPConsoleMqtt ESPConsole(Serial, "My App", "1.0");

WiFiServer server(8266);  // for remote clients

// subscribes to a relative path (relative to the set root path in the console settings)
CxMqttTopic mqttTopicTemperatureIn("temperature/in", [](const char* topic, uint8_t* payload, unsigned int len) {
   Serial.println((char*)payload);
});


void setup() {
   Serial.begin(115200);
   ESPConsole.begin(server);
   
   // publish to an absolute path
   ESPConsole.publish("/test/start", "hello world!");
   
   // publish to a relative path (relative to the set root path in the console settings)
   ESPConsole.publish("start", "hello world!");

}

void loop() {
   ESPConsole.loop();
}

