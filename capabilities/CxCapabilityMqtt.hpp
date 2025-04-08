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


#include "../tools/CxMqttManager.hpp"

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
      
      __console.info(F("====  Cap: %s  ===="), getName());
            
      __console.executeBatch(getName());

      
      _pmqttTopicCmd = new CxMqttTopic("cmd", [this](const char* topic, uint8_t* payload, unsigned int len) {
         _CONSOLE_INFO(("command is %s"), (char*)payload);
         __console.processCmd((char*)payload, true);
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
               __mqttManager.publishWill("online");
            } else {
               __console.error(F("mqtt server %s on port %d is not available!"), __mqttManager.getServer(), __mqttManager.getPort());
            }
         }
         publishInfo();
      }
   }
   
   bool execute(const char *szCmd) override {
      bool bQuiet = false;
      
      // validate the call
      if (!szCmd) return false;
      
      // get the arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
      String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      // expect sz parameter, invalid is nullptr
      const char* a = TKTOCHAR(tkArgs, 1);
      const char* b = TKTOCHAR(tkArgs, 2);
      
      if (cmd == "?") {
         printCommands();
      } else if (cmd == "mqtt") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         String strEnv = ".mqtt";
         if (strSubCmd == "connect") {
            startMqtt(TKTOCHAR(tkArgs, 2), TKTOINT(tkArgs, 3, 0));
         } else if (strSubCmd == "stop") {
            _CONSOLE_INFO(F("stop mqtt server"));
            stopMqtt();
         } else if (strSubCmd == "server") {
            __mqttManager.setServer(TKTOCHAR(tkArgs, 2));
            _bMqttServerOnline = __console.isHostAvailable(__mqttManager.getServer(), __mqttManager.getPort());
            if (!_bMqttServerOnline) println(F("server not available!"));
            startMqtt();
         } else if (strSubCmd == "port") {
            __mqttManager.setPort(TKTOINT(tkArgs, 2, 0));
            _bMqttServerOnline = __console.isHostAvailable(__mqttManager.getServer(), __mqttManager.getPort());
            if (!_bMqttServerOnline) println(F("server not available!"));
            startMqtt();
         } else if (strSubCmd == "qos") {
            __mqttManager.setQoS(TKTOINT(tkArgs, 2, 0));
         } else if (strSubCmd == "root") {
            __mqttManager.setRootPath(TKTOCHAR(tkArgs, 2));
         } else if (strSubCmd == "name") {
            __mqttManager.setName(TKTOCHAR(tkArgs, 2));
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
            }
         } else if (strSubCmd == "list") {
            __mqttManager.printSubscribtion(getIoStream());
         } else if (strSubCmd == "save") {
            CxConfigParser Config;
            Config.addVariable("server", __mqttManager.getServer());
            Config.addVariable("port", __mqttManager.getPort());
            Config.addVariable("qos", __mqttManager.getQoS());
            Config.addVariable("root", __mqttManager.getRootPath());
            Config.addVariable("name", __mqttManager.getName());
            Config.addVariable("will", (uint8_t)__mqttManager.isWill());
            Config.addVariable("willtopic", __mqttManager.getWillTopic());
            Config.addVariable("heartbeat", _timerHeartbeat.getPeriod());
            
            String strEnv = ".mqtt";
            ESPConsole.saveEnv(strEnv, Config.getConfigStr());
            
         } else if (strSubCmd == "load") {
            String strValue;
            if (ESPConsole.loadEnv(strEnv, strValue)) {
               CxConfigParser Config(strValue.c_str());
               // extract settings and set, if defined. Keep unchanged, if not set.
               __mqttManager.setServer(Config.getSz("server", __mqttManager.getServer()));
               __mqttManager.setPort(Config.getInt("port", __mqttManager.getPort()));
               __mqttManager.setQoS(Config.getInt("qos", __mqttManager.getQoS()));
               __mqttManager.setRootPath(Config.getSz("root", __mqttManager.getRootPath()));
               __mqttManager.setName(Config.getSz("name", __mqttManager.getName()));
               __mqttManager.setWill(Config.getInt("will", __mqttManager.isWill()) > 0);
               __mqttManager.setWillTopic(Config.getSz("willtopic", __mqttManager.getWillTopic()));
               int32_t period = Config.getInt("heartbeat", _timerHeartbeat.getPeriod());
               if (period == 0 || period >= 1000) _timerHeartbeat.setPeriod(period);
               _CONSOLE_INFO(F("Mqtt server set to %s at port %d, qos=%d"), __mqttManager.getServer(), __mqttManager.getPort(), __mqttManager.getQoS());
               _CONSOLE_INFO(F("Mqtt set root path to '%s' and will topic to '%s'"), __mqttManager.getRootPath(), __mqttManager.getWillTopic());
               _CONSOLE_INFO(F("Mqtt heartbeat period is set to %d"), _timerHeartbeat.getPeriod());
               _timer60sMqttServer.makeDue(); // make timer due to force an immidiate check
               
            }
         } else if (strSubCmd == "publish") {
            publish(TKTOCHAR(tkArgs, 2), TKTOCHAR(tkArgs, 3), (bool) TKTOINT(tkArgs, 4, 0));
         }
         else {
            printf(F(ESC_ATTR_BOLD " Server:       " ESC_ATTR_RESET "%s (%s)\n"), __mqttManager.getServer(), _bMqttServerOnline? ESC_TEXT_GREEN "online" ESC_ATTR_RESET: ESC_TEXT_BRIGHT_RED "offline" ESC_ATTR_RESET);
            printf(F(ESC_ATTR_BOLD " Port:         " ESC_ATTR_RESET "%d\n"), __mqttManager.getPort());
            printf(F(ESC_ATTR_BOLD " QoS:          " ESC_ATTR_RESET "%d\n"), __mqttManager.getQoS());
            printf(F(ESC_ATTR_BOLD " Root path:    " ESC_ATTR_RESET "%s\n"), __mqttManager.getRootPath());
            printf(F(ESC_ATTR_BOLD " Name:         " ESC_ATTR_RESET "%s\n"), __mqttManager.getName());
            printf(F(ESC_ATTR_BOLD " Will:         " ESC_ATTR_RESET "%s\n"), __mqttManager.isWill() ? "true" : "false");
            printf(F(ESC_ATTR_BOLD " Will topic:   " ESC_ATTR_RESET "%s\n"), __mqttManager.getWillTopic());
            printf(F(ESC_ATTR_BOLD " Heartb. per.: " ESC_ATTR_RESET "%d"), _timerHeartbeat.getPeriod()); println(F(" ms"));
            println();
#ifndef MINIMAL_HELP
            println(F(ESC_ATTR_BOLD "mqtt commands:" ESC_ATTR_RESET));
            println(F("  server <server>"));
            println(F("  port <port>"));
            println(F("  qos <qos>"));
            println(F("  root <root path>"));
            println(F("  name <name>"));
            println(F("  will <0|1> [<will topic>]"));
            println(F("  connect [<server>] [<port>]"));
            println(F("  stop"));
            println(F("  heartbeat <period in ms> (0, 1000...n)"));
            println(F("  list"));
            println(F("  save"));
            println(F("  load"));
            println(F("  publish <topic> <message> [<0|1> (retain)]"));
#endif
         }
      } else {
         return false;
      }
      g_Stack.update();
      return true;
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
         }
         // still needed?
         else {
            _CONSOLE_INFO(F("mqtt server is online!"));
            __mqttManager.publishWill("online");
         }
      } else {
         __console.error(F("mqtt server %s on port %d is not available!"), __mqttManager.getServer(), __mqttManager.getPort());
         _bMqttServerOnline = false;
      }
      return _bMqttServerOnline;
   }
   
   void stopMqtt() {
      _CONSOLE_INFO(F("stop mqtt service"));
      
      // stop timer to regular server check.
      _timer60sMqttServer.stop();
      
      __mqttManager.end();
      _bMqttServerOnline = false;
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
