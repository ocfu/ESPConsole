//
// ESPConsole.h
//

#if defined (ESP_CONSOLE_ALL)
#define ESP_CONSOLE_BASIC
#define ESP_CONSOLE_EXT
#define ESP_CONSOLE_FS
#define ESP_CONSOLE_MQTT
#define ESP_CONSOLE_MQTTHA
#define ESP_CONSOLE_I2C
#define ESP_CONSOLE_SEGDISPLAY
#define ESP_CONSOLE_RC
#endif

#if defined(ESP_CONSOLE_MQTTHA)
#include "../capabilities/CxCapabilityMqttHA.hpp"
#endif

#if defined(ESP_CONSOLE_MQTT)
#include "../capabilities/CxCapabilityMqtt.hpp"
#endif

#if defined(ESP_CONSOLE_FS)
#include "../capabilities/CxCapabilityFS.hpp"
#endif

#if defined(ESP_CONSOLE_EXT)
#include "../capabilities/CxCapabilityExt.hpp"
#endif

#if defined (ESP_CONSOLE_BASIC)
#include "../capabilities/CxCapabilityBasic.hpp"
#endif

#if defined (ESP_CONSOLE_I2C)
#include "../capabilities/CxCapabilityI2C.hpp"
#endif

#if defined (ESP_CONSOLE_SEGDISPLAY)
#include "../capabilities/CxCapabilitySegDisplay.hpp"
#endif

#if defined (ESP_CONSOLE_RC)
#include "../capabilities/CxCapabilityRC.hpp"
#endif


#ifndef __SKIP_GLOBALS__
#define __SKIP_GLOBALS__
#pragma GCC push_options
#pragma GCC optimize ("O0")
#if defined (BUILD_ID) && defined (_VERSION)
#define _VERSION_ID _VERSION "(" BUILD_ID ")"
#elif defined (_VERSION)
#define _VERSION_ID _VERSION
#else
#define _VERSION_ID "-"
#endif
// for the identification of the binary (e.g. for archiving purposes)
#if defined (_NAME) && defined (_VERSION)
const char* g_szId      PROGMEM = "$$id:" _NAME ":" _VERSION;
#endif
#if defined (ESPCONSOLE_VERSION)
const char* g_szIdmyESP PROGMEM = "$$idm:myESP:" ESPCONSOLE_VERSION;
#endif
#ifndef _NAME
#define _NAME "App"
#endif
#pragma GCC pop_options


void initESPConsole(const char* app = _NAME, const char* ver = _VERSION_ID) {
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
#ifdef CxCapabilityRC_hpp
   CxCapabilityRC::loadCap();
#endif

}


#endif /*__SKIP_GLOBALS__ */

