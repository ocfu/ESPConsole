#if defined (ESP_CONSOLE_BASIC)
#include "../capabilities/CxCapabilityBasic.hpp"
#elif defined(ESP_CONSOLE_EXT)
#include "../capabilities/CxCapabilityExt.hpp"
#elif defined(ESP_CONSOLE_FS)
#include "../capabilities/CxCapabilityFS.hpp"
#elif defined(ESP_CONSOLE_MQTT)
#include "../capabilities/CxCapabilityMqtt.hpp"
#elif defined(ESP_CONSOLE_MQTTHA)
#include "../capabilities/CxCapabilityMqttHA.hpp"
#endif

#if defined (ESP_CONSOLE_ALL)
#include "../capabilities/CxCapabilityMqttHA.hpp"
#endif

#if defined (ESP_CONSOLE_I2C) || defined (ESP_CONSOLE_ALL)
#include "../capabilities/CxCapabilityI2C.hpp"
#endif

#if defined (ESP_CONSOLE_SEGDISPLAY) || defined (ESP_CONSOLE_ALL)
#include "../capabilities/CxCapabilitySegDisplay.hpp"
#endif

void initESPConsole(const char* app, const char* ver) {
   g_Stack.begin();
   
#ifdef ARDUINO
   Serial.begin(115200);
   Serial.println();
#endif
   
   ESPConsole.setAppNameVer(app, ver);
   
#ifdef CxCapabilityBasic_hpp
   CxCapabilityBasic::loadCap();
#endif
#ifdef CxCapabilityExt_hpp
   CxCapabilityExt::loadCap();
#endif
#ifdef CxCapabilityFS_hpp
   CxCapabilityFS::loadCap();
#endif
#ifdef CxCapabilityI2C_hpp
   CxCapabilityI2C::loadCap();
#endif
#ifdef CxCapabilityMqtt_hpp
   CxCapabilityMqtt::loadCap();
#endif
#ifdef CxCapabilityMqttHA_hpp
   CxCapabilityMqttHA::loadCap();
#endif
#ifdef CxCapabilitySegDisplay_hpp
   CxCapabilitySegDisplay::loadCap();
#endif
}



