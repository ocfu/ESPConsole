//
//  CxESPConsoleMqtt.hpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPConsoleMqtt_hpp
#define CxESPConsoleMqtt_hpp

#ifdef ESP_CONSOLE_NOWIFI
#error "ESP_CONSOLE_NOWIFI was defined. CxESPConsoleMqtt requires a network to work! Please change to another CxESPConsole."
#endif

#include "CxESPConsoleLog.hpp"
#include "../tools/CxMqttManager.hpp"

#ifdef ARDUINO
#include <PubSubClient.h>

#ifdef ESP32
#else  // no ESP32
#endif // end ESP32
#endif // end ARDUINO

class CxESPConsoleMqtt : public CxESPConsoleLog {
private:
   CxMqttManager& _mqttManager = CxMqttManager::getInstance();

   bool _bMqttServerOnline = false;
   
   CxTimer    _timerHeartbeat;
   CxTimer60s _timer60sMqttServer;
   
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleMqtt(wifiClient, app, ver);
   }
   
   CxMqttTopic* _pmqttTopicCmd = nullptr;
   
protected:
   virtual bool __processCommand(const char* szCmd, bool bQuiet = false) override;

public:
   CxESPConsoleMqtt(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsoleMqtt((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
   CxESPConsoleMqtt(Stream& stream, const char* app = "", const char* ver = "") : CxESPConsoleLog(stream, app, ver), _timer60sMqttServer(true), _timerHeartbeat(0) {} // put timer on hold

   using CxESPConsole::begin;
   virtual void begin() override;
   // specifics for this console class, when needed
   //virtual void end() override {CxESPConsoleLog::end();}
   virtual void loop() override;
   
   virtual void printInfo() override;
   
   bool startMqtt(const char* server = nullptr, uint32_t port = 0);
   void stopMqtt();
   bool connectMqtt();
   void disconnectMqtt();
   bool isConnectedMqtt();
   
   bool subscribe(const char* topic, CxMqttManager::tCallback callback) {
      return _mqttManager.subscribe(topic, callback);
   }

   bool publish(const char* topic, const char* payload, bool retained = false) {
      return _mqttManager.publish(topic, payload, retained);
   }
   bool publish(const FLASHSTRINGHELPER * topicP, const char* payload, bool retained = false) {
      char buf[256];
      strncpy_P(buf, (PGM_P)topicP, sizeof(buf));
      buf[sizeof(buf)-1] = '\0';
      return publish(buf, payload, retained);
   };

   void publishInfo();
   
};

#endif /* CxESPConsoleMqtt_hpp */
