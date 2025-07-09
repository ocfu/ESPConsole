/**
 * @file CxCapabilityMqtt.hpp
 * @brief MQTT Capability for ESP-based projects
 *
 * This file defines the `CxCapabilityMqtt` class, which provides MQTT capabilities
 * for an ESP-based project. It includes methods for setting up, managing, and
 * executing MQTT-related commands.
 *
 * @date 09.01.25
 * @author ocfu
 * @copyright © 2025 ocfu
 */

#ifndef CxCapabilityMqtt_hpp
#define CxCapabilityMqtt_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityFS.hpp"


#include "tools/CxMqttManager.hpp"

class CxCapabilityMqtt : public CxCapability {
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();
   
   bool _bMqttServerOnline = false;
   
   CxTimer    _timerHeartbeat;
   CxTimer60s _timer60sMqttServer;

   CxMqttTopic* _pmqttTopicCmd = nullptr;
   
   CxMqttManager& __mqttManager = CxMqttManager::getInstance();

   
public:
   explicit CxCapabilityMqtt()
   : CxCapability("mqtt", getCmds()) {}
   static constexpr const char* getName() { return "mqtt"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "mqtt" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityMqtt>();
   }
   
   ~CxCapabilityMqtt() {
      if (_pmqttTopicCmd) delete _pmqttTopicCmd;
      _pmqttTopicCmd = nullptr;
   }
   
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*__console.getStream());
      __bLocked = false;
      
      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());
      
      __console.executeBatch("init", getName());

      
      _pmqttTopicCmd = new CxMqttTopic("cmd", [this](const char* topic, uint8_t* payload, unsigned int len) {
         _CONSOLE_INFO(("command is %s"), (char*)payload);
         __console.processCmd((char*)payload);
      });

      
      _timerHeartbeat.start(true); // 1st due immidiately

   }
   
   void loop() override {
      if (__console.isConnected()) {
         if (_timerHeartbeat.isDue()) {
            __mqttManager.publish("heartbeat", String((uint32_t)millis()).c_str());
         }
         __mqttManager.loop();
      }
      if (_timer60sMqttServer.isDue()) {
         bool bOnline = _bMqttServerOnline;
         _bMqttServerOnline = __console.isHostAvailable(__mqttManager.getServer(), __mqttManager.getPort());
         if (_bMqttServerOnline != bOnline) {
            if (bOnline) {
               _CONSOLE_INFO(F("mqtt server is online!"));
            } else {
               __console.error(F("mqtt server %s on port %d is not available!"), __mqttManager.getServer(), __mqttManager.getPort());
            }
         }
         publishInfo();
      }
   }
   
   uint8_t execute(const char *szCmd, uint8_t nClient) override {
      
      // validate the call
      if (!szCmd) return EXIT_FAILURE;
      
      // get the arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
      String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      uint8_t nExitValue = EXIT_FAILURE;

      // expect sz parameter, invalid is nullptr
      const char* b = TKTOCHAR(tkArgs, 2);
      
      if (cmd == "?") {
         nExitValue = printCommands();
      } else if (cmd == "mqtt") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         String strEnv = ".mqtt";
         nExitValue = EXIT_SUCCESS;
         if (strSubCmd == "connect") {
            if (!startMqtt(TKTOCHAR(tkArgs, 2), TKTOINT(tkArgs, 3, 0))) {
               nExitValue = EXIT_FAILURE;
            };
         } else if (strSubCmd == "stop") {
            _CONSOLE_INFO(F("stop mqtt server"));
            nExitValue = stopMqtt();
         } else if (strSubCmd == "server") {
            __mqttManager.setServer(TKTOCHAR(tkArgs, 2));
            __mqttManager.setPort(TKTOINT(tkArgs, 3, 8880));
         } else if (strSubCmd == "qos") {
            __mqttManager.setQoS(TKTOINT(tkArgs, 2, 0));
         } else if (strSubCmd == "root") {
            __mqttManager.setRootPath(TKTOCHAR(tkArgs, 2));
         } else if (strSubCmd == "name") {
            __mqttManager.setName(TKTOCHARAFTER(tkArgs, 2));
         } else if (strSubCmd == "heartbeat") {
            int32_t period = TKTOINT(tkArgs, 2, -1);
            if (period == 0 || period >= 1000) _timerHeartbeat.start(period, true);
         } else if (strSubCmd == "will") {
            if (b) {
               int8_t nWill = (int8_t)TKTOINT(tkArgs, 2, -1);
               // disable or enable will behaviour.
               __mqttManager.setWill(nWill > 0);
               // set topic. If no topic was set, the root path is used as will topic
               __mqttManager.setWillTopic(TKTOCHAR(tkArgs, 3));
            } else {
               nExitValue = EXIT_FAILURE;
            }
         } else if (strSubCmd == "list") {
            __mqttManager.printSubscribtion(getIoStream());
         } else if (strSubCmd == "counter") {
            __console.setOutputVariable(__mqttManager.getConnectCntr());
         } else if (strSubCmd == "publish") {
            publish(TKTOCHAR(tkArgs, 2), TKTOCHAR(tkArgs, 3), (bool) TKTOINT(tkArgs, 4, 0));
         } else if (strSubCmd == "pubvar") {
            publishVariables(TKTOCHAR(tkArgs, 2));
         }
         else if (strSubCmd == "subscribe") {
            // subscribe <topic> <variable> [<command>]
            CxMqttTopic* pMqttTopic = nullptr;

            if (TKTOCHAR(tkArgs, 2) && !__mqttManager.findTopic(TKTOCHAR(tkArgs, 2))) {
               
               // FIXME: store and enable deletion of the object
               
               pMqttTopic = new CxMqttTopic(TKTOCHAR(tkArgs, 2), [this](const char* topic, uint8_t* payload, unsigned int length) {
                  const char* szCmd = __mqttManager.getCmd(topic);
                  const char* szVar = __mqttManager.getVariable(topic);
                  const char* szPayload = (char*) payload;
                  
                  String strValue;
                  
                  if (szPayload && *szPayload == '{') {
                     DynamicJsonDocument doc(1024);
                     DeserializationError error = deserializeJson(doc, szPayload);
                     if (!error) {
                        strValue = doc["value"] | "";
                     }
                  } else {
                     strValue = szPayload;
                  }
                  
                  if (szVar) {
                     __console.addVariable(szVar, strValue.c_str());
                  }
                  
                  if (szCmd) {
                     __console.processCmd(szCmd);
                  }
                  
               }, false, false); // not retained, not auto subscribed
            }
            if (pMqttTopic) {
               pMqttTopic->setVariable(TKTOCHAR(tkArgs, 3));
               pMqttTopic->setCmd(TKTOCHAR(tkArgs, 4));
               pMqttTopic->subscribe();
            } else {
               nExitValue = EXIT_FAILURE;
            }
         }
         else {
            printf(F(ESC_ATTR_BOLD " Server:       " ESC_ATTR_RESET "%s (%s)\n"), __mqttManager.getServer(), _bMqttServerOnline? ESC_TEXT_GREEN "online" ESC_ATTR_RESET: ESC_TEXT_BRIGHT_RED "offline" ESC_ATTR_RESET);
            printf(F(ESC_ATTR_BOLD " Port:         " ESC_ATTR_RESET "%d\n"), __mqttManager.getPort());
#ifndef MINIMAL_COMMAND_SET

            printf(F(ESC_ATTR_BOLD " QoS:          " ESC_ATTR_RESET "%d\n"), __mqttManager.getQoS());
            printf(F(ESC_ATTR_BOLD " Root path:    " ESC_ATTR_RESET "%s\n"), __mqttManager.getRootPath());
            printf(F(ESC_ATTR_BOLD " Name:         " ESC_ATTR_RESET "%s\n"), __mqttManager.getName());
            printf(F(ESC_ATTR_BOLD " Will:         " ESC_ATTR_RESET "%s\n"), __mqttManager.isWill() ? "true" : "false");
            printf(F(ESC_ATTR_BOLD " Will topic:   " ESC_ATTR_RESET "%s\n"), __mqttManager.getWillTopic());
            printf(F(ESC_ATTR_BOLD " Heartb. per.: " ESC_ATTR_RESET "%d"), _timerHeartbeat.getPeriod()); println(F(" ms"));
#endif
            println();
            __console.man(getName());
         }
      } else {
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }
      
   bool subscribe(const char* topic, CxMqttManager::tCallback callback) {
      return __mqttManager.subscribe(topic, callback);
   }
   
   bool publish(const char* topic, const char* payload, bool retained = false) {
      return __mqttManager.publish(topic, payload, retained);
   }
   bool publish(const FLASHSTRINGHELPER * topicP, const char* payload, bool retained = false) {
      static char buf[256];
      strncpy_P(buf, (PGM_P)topicP, sizeof(buf));
      buf[sizeof(buf)-1] = '\0';
      return publish(buf, payload, retained);
   };
   
   void publishVariables(const char* szParam) {
      for (const auto& entry : __console.getVariables()) {
         if (isalpha(entry.first.charAt(0))) {
            String strPath;
            strPath.reserve(128);
            strPath = F("variables/");
            strPath += entry.first;
            publish(strPath.c_str(), entry.second.c_str());
         }
      }
   }
   
   bool startMqtt(const char* server = nullptr, uint32_t port = 0) {
      stopMqtt();
      
      // start timer to regular server check.
      _timer60sMqttServer.start();
      
      if (server) __mqttManager.setServer(server);
      if (port > 0) __mqttManager.setPort(port);
      if (__console.isHostAvailable(__mqttManager.getServer(), __mqttManager.getPort())) {
         _CONSOLE_INFO(F("start mqtt service"));
         _CONSOLE_INFO(F("connecting mqtt server %s on port %d"), __mqttManager.getServer(), __mqttManager.getPort());
         if (__mqttManager.getRootPath()) {
            _CONSOLE_INFO(F("root path is '%s'"), __mqttManager.getRootPath() ? __mqttManager.getRootPath() : "");
         }
         if (__mqttManager.isWill()) {
            if (__mqttManager.getWillTopic() && __mqttManager.getWillMessage()) {
               _CONSOLE_INFO(F("last will message is '%s' on topic '%s'"), __mqttManager.getWillMessage(), __mqttManager.getWillTopic());
            }
         } else {
            _CONSOLE_INFO(F("no last will was set."));
         }
         _bMqttServerOnline = __mqttManager.begin();
         if (!_bMqttServerOnline) {
            __console.error(F("connecting mqtt server failed!"));
         } else {
            _CONSOLE_INFO(F("mqtt server is online!"));
            __mqttManager.publishWill("online");
         }
      } else {
         __console.error(F("mqtt server %s on port %d is not available!"), __mqttManager.getServer(), __mqttManager.getPort());
         _bMqttServerOnline = false;
      }
      return _bMqttServerOnline;
   }
   
   uint8_t stopMqtt() {
      _CONSOLE_INFO(F("stop mqtt service"));
      
      // stop timer to regular server check.
      _timer60sMqttServer.stop();
      
      __mqttManager.end();
      _bMqttServerOnline = false;
      return EXIT_SUCCESS;
   }
   
   bool isConnectedMqtt() {
      return __console.isConnected() && __mqttManager.isConnected();
   }
      
   void publishInfo() {
      if (isConnectedMqtt()) {
         publish(F("info/freemem"), getFreeHeap());
         publish(F("info/fragmentation"), getHeapFragmentation());
         publish(F("info/uptime"), __console.getUpTimeISO());
         publish(F("info/name"), __mqttManager.getName());
         publish(F("info/hostname"), __console.getHostName());
         //      publish(F("info/looptime"), Time1.getLoopTimeAvr());
         //      publish(F("info/chip"), CxTools::getChipInfo());
         //      publish(F("info/rssi"), "%d", Wifi1.getRSSI());
         //      publish(F("info/df"), CxSpiffs::df());
         //      publish(F("info/freeota"), CxTools::getFreeOTA());
         
      }
   }


   static void loadCap() {
      CAPREG(CxCapabilityMqtt);
      CAPLOAD(CxCapabilityMqtt);
  };

};

#endif /* CxCapabilityMqtt_hpp */
