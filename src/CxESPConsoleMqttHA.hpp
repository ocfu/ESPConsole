//
//  CxESPConsoleMqttHA.hpp
//  xESP
//
//  Created by ocfu on 01.02.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#ifndef CxESPConsoleMqttHA_hpp
#define CxESPConsoleMqttHA_hpp

#ifdef ESP_CONSOLE_NOWIFI
#error "ESP_CONSOLE_NOWIFI was defined. CxESPConsoleMqtt requires a network to work! Please change to another CxESPConsole."
#endif

#include "CxESPConsoleMqtt.hpp"
#include "CxMqttHAManager.hpp"

#ifdef ARDUINO

#ifdef ESP32
#else  // no ESP32
#endif // end ESP32
#endif // end ARDUINO

class CxESPConsoleMqttHA : public CxESPConsoleMqtt {
private:
   CxMqttHADevice& _mqttHAdev = CxMqttHADevice::getInstance();
   
   bool _bHAEnabled;

   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleMqttHA(wifiClient, app, ver);
   }
   
   bool _processCommand(const char* szCmd, bool bQuiet = false);

protected:

public:
   CxESPConsoleMqttHA(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsoleMqttHA((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
   CxESPConsoleMqttHA(Stream& stream, const char* app = "", const char* ver = "") : CxESPConsoleMqtt(stream, app, ver), _bHAEnabled(true) {

      // register commmand set for this class
      commandHandler.registerCommandSet(F("Home Assistant"), [this](const char* cmd, bool bQuiet)->bool {return _processCommand(cmd, bQuiet);}, F("ha"), F("Home Assistant commands"));

   }
   
   bool isEnabled() {return _bHAEnabled;}
   void setEnabled(bool set) {_bHAEnabled = set;}

   using CxESPConsole::begin;
   virtual void begin() override;
   // specifics for this console class, when needed
   //virtual void end() override {CxESPConsoleMqtt::end();}
   virtual void loop() override;
   
   virtual void printInfo() override;
   
   void enableHA(bool enabled);
      
};

#endif /* CxESPConsoleMqttHA_hpp */
