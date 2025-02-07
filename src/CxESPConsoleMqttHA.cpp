//
//  CxESPConsoleMqttHA.cpp
//  xESP
//
//  Created by ocfu on 01.02.25.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleMqttHA.hpp"
#include "CxConfigParser.hpp"

#ifndef ESP_CONSOLE_NOFS
void CxESPConsoleMqttHA::begin() {
   // set the name for this console
   setConsoleName("MQTT HA");
   
   // increase the PubSubClient buffer size as for HA the payload could be pretty long, especially for discovery topics.
   __mqttManager.setBufferSize(1024);

   // call the begin() from base class(es)
   CxESPConsoleMqtt::begin();
   
   info(F("=== MQTT HA ==="));
   
#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient && !isConnected()) startWiFi();
#endif

   // load specific environments for this class
   mount();
   _processCommand("ha load");
   
   if (!__isWiFiClient()) {
      if (_bHAEnabled) enableHA(true);
      
      info(F("mqtt ha started"));
   }
}

void CxESPConsoleMqttHA::printInfo() {
   CxESPConsoleMqtt::printInfo();
   
   // specific for this console
}

bool CxESPConsoleMqttHA::_processCommand(const char *szCmd, bool bQuiet) {
   // validate the call
   if (!szCmd) return false;
   
   // get the command and arguments into the token buffer
   CxStrToken tkCmd(szCmd, " ");
   
   // validate again
   if (!tkCmd.count()) return false;
      
   // we have a command, find the action to take
   String strCmd = TKTOCHAR(tkCmd, 0);
   
   // removes heading and trailing white spaces
   strCmd.trim();
   
   // expect sz parameter, invalid is nullptr
   const char* a = TKTOCHAR(tkCmd, 1);
   const char* b = TKTOCHAR(tkCmd, 2);
   
   if (strCmd == "ha") {
      String strSubCmd = TKTOCHAR(tkCmd, 1);
      String strEnv = ".ha";
      if (strSubCmd == "enable") {
         _bHAEnabled = (bool)TKTOINT(tkCmd, 2, 0);
         enableHA(_bHAEnabled);
      } else if (strSubCmd == "list") {
         _mqttHAdev.printList(*__ioStream);
      } else if (strSubCmd == "save") {
         CxConfigParser Config;
         Config.addVariable("enabled", _bHAEnabled);
         saveEnv(strEnv, Config.getConfigStr());
      } else if (strSubCmd == "load") {
         String strValue;
         if (loadEnv(strEnv, strValue)) {
            CxConfigParser Config(strValue);
            // extract settings and set, if defined. Keep unchanged, if not set.
            _bHAEnabled = Config.getBool("enabled", _bHAEnabled);
            info(F("Mqtt HA support enabled: %d"), _bHAEnabled);
         }
      } else {
         printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), _bHAEnabled);
         println(F("ha commands:"));
         println(F("  enable 0|1"));
         println(F("  list"));
         println(F("  save"));
         println(F("  load"));
      }
   } else {
      return false;
   }
   return true;
}

void CxESPConsoleMqttHA::loop() {
   CxESPConsoleMqtt::loop();
   if (isConnected()) {      
   }
}

void CxESPConsoleMqttHA::enableHA(bool enabled) {
   _mqttHAdev.setFriendlyName(getAppName());
   _mqttHAdev.setName(getAppName());
   _mqttHAdev.setTopicBase("ha");
   _mqttHAdev.setManufacturer("ocfu");
   _mqttHAdev.setModel("my Model");
   _mqttHAdev.setSwVersion(getAppVer());
   _mqttHAdev.setHwVersion("ESP");
   _mqttHAdev.setUrl("");
   _mqttHAdev.setStrId();

   _mqttHAdev.regItems(enabled);
   _mqttHAdev.publishAvailability(enabled);
}

#endif /*ESP_CONSOLE_NOFS*/
