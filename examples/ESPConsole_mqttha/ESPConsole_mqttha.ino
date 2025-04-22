#include "CxESPConsole.hpp"
#include "../capabilities/CxCapabilityMqttHA.hpp"
#include "../capabilities/CxCapabilityI2C.hpp"
#include "../tools/CxSensorBme.hpp"
#include "../capabilities/CxCapabilitySegDisplay.hpp"

// ----------------------------------------------------------------------------
// BME Sensor
//
CxSensorBme g_Temperature ("temp-in");
CxSensorBme g_Humidity;
CxSensorBme g_Pressure;

void startBme() {
   CxCapabilityI2C* pI2C = CxCapabilityI2C::getInstance();
   
   if (pI2C) {
      ESPConsole.debug(F("initialise BME sensors..."));
      g_Temperature.begin(pI2C->getBmeDevice(), ECSensorType::temperature);
      g_Humidity.begin(pI2C->getBmeDevice(), ECSensorType::humidity);
      g_Pressure.begin(pI2C->getBmeDevice(), ECSensorType::pressure);
   }
}


CxMqttHASensor haSensorTemperature("Temperatur", "temperature", "temperature", "°C");
CxMqttHASensor haSensorTemperatureGarden("Temperatur Garden", "temperature_garden", "temperature", "°C");

// subscribes to a relative path (relative to the set root path in the console settings)
CxMqttTopic mqttTopicTemperature("temperature", [](const char* topic, uint8_t* payload, unsigned int len) {
   Serial.print(topic);
   Serial.print(": ");
   Serial.println((char*)payload);
   haSensorTemperature.publishState((char*)payload);
});

// subscribes to an absolute path
CxMqttTopic mqttTopicTemperatureGarden("/sensors/garden/temperature", [](const char* topic, uint8_t* payload, unsigned int len) {
   Serial.print(topic);
   Serial.print(": ");
   Serial.println((char*)payload);
   haSensorTemperatureGarden.publishState((char*)payload);
});


WiFiServer server(8266);

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

CXCAPABILITY(CxMyCapability, "app", "abc");

void CxMyCapability::setup(){
   println(F("app cap setup called"));
}
void CxMyCapability::loop(){
   static CxTimer60s timer;
   
   if (timer.isDue()) {
      if (g_Temperature.hasValidValue()) {
         haSensorTemperature.publishState(g_Temperature.getFloatValue(), 2);
      } else {
         haSensorTemperature.publishAvailability(false);
      }
   }
}

bool CxMyCapability::execute(const char *cmd) {
   if (*cmd == '\?') {
      printCommands();
   } else if (strncmp(cmd, "abc", 4) == 0) {
      print(F("T="));print(g_Temperature.getFloatValue());println(g_Temperature.getUnit());
   } else {
      return false;
   }
   return true;
}

void setup() {
   g_Stack.begin();
   
   Serial.begin(115200);
   Serial.println();
   
   ESPConsole.setAppNameVer("test 1", "v1.0");
   CAPREG(CxMyCapability);
   CAPLOAD(CxMyCapability);
   
   CAPREG(CxCapabilityBasic);
   CAPLOAD(CxCapabilityBasic);
   
   CAPREG(CxCapabilityExt);
   CAPLOAD(CxCapabilityExt);
   
   CAPREG(CxCapabilityFS);
   CAPLOAD(CxCapabilityFS);
   
   CAPREG(CxCapabilityMqtt);
   CAPLOAD(CxCapabilityMqtt);
   
   CAPREG(CxCapabilityMqttHA);
   CAPLOAD(CxCapabilityMqttHA);
   
   CAPREG(CxCapabilityI2C);
   CAPLOAD(CxCapabilityI2C);
   startBme();
   ESPConsole.processCmd("sensor list");
   
   CAPREG(CxCapabilitySegDisplay);
   CAPLOAD(CxCapabilitySegDisplay);
   
   
   ESPConsole.begin(server);
   
   
}

void loop() {
   ESPConsole.loop();
}


#endif


