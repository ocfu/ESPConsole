//
//  CxESPConsoleMqtt.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleMqtt.hpp"
#include "../tools/CxConfigParser.hpp"

#ifndef ESP_CONSOLE_NOFS
void CxESPConsoleMqtt::begin() {
   // set the name for this console
   setConsoleName("MQTT");
   
   // call the begin() from base class(es)
   CxESPConsoleLog::begin();
   
   info(F("==== MQTT ===="));
   
#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient && !isConnected()) startWiFi();
#endif

   // load specific environments for this class
   mount();
   __processCommand("mqtt load");
   
   if (!__isWiFiClient()) {

      /// MARK: IDEA: configured subscribtions
      /// subscribtions defined in a config file
      ///   <variable>=<topic>
      ///   subscribe on path. introduce a "tag" using variable
      ///   compare with the "tag"
      ///   maintain a map with the pair <variable>:<value>
      ///   create a method to read the value
      ///
      ///
      
      _pmqttTopicCmd = new CxMqttTopic("cmd", [this](const char* topic, uint8_t* payload, unsigned int len) {
         info(("command is %s"), (char*)payload);
         __processCommand((char*)payload, true);
      });
      
      /*
      /// Avoid using short living CxMqttTopic objects!!
      CxMqttTopic mqttTest("my name", "info");
      mqttTest.publish("hello");
      
      /// prefer this
      this->publish("info", "hello"); // using CxMqttTopic for publish topics makes only sense, if it is used frequently or the name is relevant (e.g. for HA)
       
      */
      
      startMqtt();

      info(F("mqtt started"));
      _timerHeartbeat.start(true); // 1st due immidiately
   }
}

void CxESPConsoleMqtt::printInfo() {
   CxESPConsoleLog::printInfo();
   
   // specific for this console
}

bool CxESPConsoleMqtt::__processCommand(const char *szCmd, bool bQuiet) {
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
   
   if (strCmd == "?" || strCmd == USR_CMD_HELP) {
      // show help first from base class(es)
      CxESPConsoleLog::__processCommand(szCmd);
      println(F("Mqtt commands:" ESC_TEXT_BRIGHT_WHITE "    mqtt" ESC_ATTR_RESET));
   } else if (strCmd == "mqtt") {
      String strSubCmd = TKTOCHAR(tkCmd, 1);
      String strEnv = ".mqtt";
      if (strSubCmd == "connect") {
         startMqtt(TKTOCHAR(tkCmd, 2), TKTOINT(tkCmd, 3, 0));
      } else if (strSubCmd == "stop") {
         info(F("stop mqtt server"));
         stopMqtt();
      } else if (strSubCmd == "server") {
         _mqttManager.setServer(TKTOCHAR(tkCmd, 2));
         _bMqttServerOnline = isHostAvailble(_mqttManager.getServer(), _mqttManager.getPort());
         if (!_bMqttServerOnline) println(F("server not available!"));
         startMqtt();
      } else if (strSubCmd == "port") {
         _mqttManager.setPort(TKTOINT(tkCmd, 2, 0));
         _bMqttServerOnline = isHostAvailble(_mqttManager.getServer(), _mqttManager.getPort());
         if (!_bMqttServerOnline) println(F("server not available!"));
         startMqtt();
      } else if (strSubCmd == "qos") {
         _mqttManager.setQoS(TKTOINT(tkCmd, 2, 0));
      } else if (strSubCmd == "root") {
         _mqttManager.setRootPath(TKTOCHAR(tkCmd, 2));
      } else if (strSubCmd == "heartbeat") {
         int32_t period = TKTOINT(tkCmd, 2, -1);
         if (period == 0 || period >= 1000) _timerHeartbeat.start(period, true);
      } else if (strSubCmd == "will") {
         if (b) {
            int8_t bWill = (int8_t)TKTOINT(tkCmd, 2, -1);
            if (bWill > 0) {
               // disable or enable will behaviour. In case to will topic was set, the will topic is the rootPath
               _mqttManager.setWill(bWill);
            } else {
               // set topic. will behaviour implicitly enabled, if topic length > 0.
               _mqttManager.setWillTopic(TKTOCHAR(tkCmd, 2));
            }
         }
      } else if (strSubCmd == "list") {
         _mqttManager.printSubscribtion(*__ioStream);
      } else if (strSubCmd == "save") {
         CxConfigParser Config;
         Config.addVariable("server", _mqttManager.getServer());
         Config.addVariable("port", _mqttManager.getPort());
         Config.addVariable("qos", _mqttManager.getQoS());
         Config.addVariable("root", _mqttManager.getRootPath());
         Config.addVariable("will", (uint8_t)_mqttManager.isWill());
         Config.addVariable("willtopic", _mqttManager.getWillTopic());
         Config.addVariable("heartbeat", _timerHeartbeat.getPeriod());
         saveEnv(strEnv, Config.getConfigStr());
      } else if (strSubCmd == "load") {
         String strValue;
         if (loadEnv(strEnv, strValue)) {
            CxConfigParser Config(strValue);
            // extract settings and set, if defined. Keep unchanged, if not set.
            _mqttManager.setServer(Config.getSz("server", _mqttManager.getServer()));
            _mqttManager.setPort(Config.getInt("port", _mqttManager.getPort()));
            _mqttManager.setQoS(Config.getInt("qos", _mqttManager.getQoS()));
            _mqttManager.setRootPath(Config.getSz("root", _mqttManager.getRootPath()));
            _mqttManager.setWill(Config.getInt("will", _mqttManager.isWill()) > 0);
            _mqttManager.setWillTopic(Config.getSz("willtopic", _mqttManager.getWillTopic()));
            int32_t period = Config.getInt("heartbeat", _timerHeartbeat.getPeriod());
            if (period == 0 || period >= 1000) _timerHeartbeat.setPeriod(period);
            info(F("Mqtt server set to %s at port %d, qos=%d"), _mqttManager.getServer(), _mqttManager.getPort(), _mqttManager.getQoS());
            info(F("Mqtt set root path to '%s' and will topic to '%s'"), _mqttManager.getRootPath(), _mqttManager.getWillTopic());
            info(F("Mqtt heartbeat period is set to %d"), _timerHeartbeat.getPeriod());
            _timer60sMqttServer.makeDue(); // make timer due to force an immidiate check
         }
      } else {
         printf(F(ESC_ATTR_BOLD " Server:      " ESC_ATTR_RESET "%s (%s)\n"), _mqttManager.getServer(), _bMqttServerOnline?"online":"offline");
         printf(F(ESC_ATTR_BOLD " Port:        " ESC_ATTR_RESET "%d\n"), _mqttManager.getPort());
         printf(F(ESC_ATTR_BOLD " QoS:         " ESC_ATTR_RESET "%d\n"), _mqttManager.getQoS());
         printf(F(ESC_ATTR_BOLD " Root path:   " ESC_ATTR_RESET "%s\n"), _mqttManager.getRootPath());
         printf(F(ESC_ATTR_BOLD " Will:        " ESC_ATTR_RESET "%d\n"), _mqttManager.isWill());
         printf(F(ESC_ATTR_BOLD " Will topic:  " ESC_ATTR_RESET "%s\n"), _mqttManager.getWillTopic());
         printf(F(ESC_ATTR_BOLD " Heatb. per.: " ESC_ATTR_RESET "%d\n"), _timerHeartbeat.getPeriod());
         println(F("mqtt commands:"));
         println(F("  server <server>"));
         println(F("  port <port>"));
         println(F("  qos <qos>"));
         println(F("  root <root path>"));
         println(F("  will <0|1> | <will topic>"));
         println(F("  connect [<server>] [<port>]"));
         println(F("  stop"));
         println(F("  hearbeat <period in ms> (0, 1000...n)"));
         println(F("  list"));
         println(F("  save"));
         println(F("  load"));
      }
   } else {
      // command not handled here, proceed into the base class
      return CxESPConsoleLog::__processCommand(szCmd, bQuiet);
   }
   return true;
}

bool CxESPConsoleMqtt::startMqtt(const char* server, uint32_t port) {
   stopMqtt();

   // start timer to regular server check.
   _timer60sMqttServer.start();
   
   if (server) _mqttManager.setServer(server);
   if (port > 0) _mqttManager.setPort(port);
   if (isHostAvailble(_mqttManager.getServer(), _mqttManager.getPort())) {
      info(F("start mqtt service"));
      info(F("connecting mqtt server %s on port %d"), _mqttManager.getServer(), _mqttManager.getPort());
      if (_mqttManager.getRootPath()) {
         info(F("root path is '%s'"), _mqttManager.getRootPath() ? _mqttManager.getRootPath() : "");
      }
      if (_mqttManager.isWill()) {
         if (_mqttManager.getWillTopic() && _mqttManager.getWillMessage()) {
            info(F("last will message is '%s' on topic '%s'"), _mqttManager.getWillMessage(), _mqttManager.getWillTopic());
         }
      } else {
         info(F("no last will was set."));
      }
      _bMqttServerOnline = _mqttManager.begin();
      if (!_bMqttServerOnline) {
         error(F("connecting mqtt server failed!"));
      }
      // still needed?
      else {
         info(F("mqtt server is online!"));
         _mqttManager.publishWill("online");
      }
   } else {
      error(F("mqtt server %s on port %d is not available!"), _mqttManager.getServer(), _mqttManager.getPort());
      _bMqttServerOnline = false;
   }
   return _bMqttServerOnline;
}

void CxESPConsoleMqtt::stopMqtt() {
   info(F("stop mqtt service"));
   
   // stop timer to regular server check.
   _timer60sMqttServer.stop();
   
    _mqttManager.end();
   _bMqttServerOnline = false;
}

bool CxESPConsoleMqtt::isConnectedMqtt() {
   return isConnected() && _mqttManager.isConnected();
}

void CxESPConsoleMqtt::loop() {
   CxESPConsoleLog::loop();
   if (isConnected()) {      
      if (_timerHeartbeat.isDue()) {
         _mqttManager.publish("heartbeat", String((uint32_t)millis()).c_str());
      }
      _mqttManager.loop();
   }
   if (_timer60sMqttServer.isDue()) {
      bool bOnline = _bMqttServerOnline;
      _bMqttServerOnline = isHostAvailble(_mqttManager.getServer(), _mqttManager.getPort());
      if (_bMqttServerOnline != bOnline) {
         if (bOnline) {
            info(F("mqtt server is online!"));
            _mqttManager.publishWill("online");
         } else {
            error(F("mqtt server %s on port %d is not available!"), _mqttManager.getServer(), _mqttManager.getPort());
         }
      }
      publishInfo();
   }
}

void CxESPConsoleMqtt::publishInfo() {
   if (isConnectedMqtt()) {
      publish(F("info/freemem"), getFreeHeap());
      publish(F("info/fragmentation"), getHeapFragmentation());
      publish(F("info/uptime"), getUpTimeISO());
//      publish(F("info/looptime"), Time1.getLoopTimeAvr());
//      publish(F("info/chip"), CxTools::getChipInfo());
//      publish(F("info/rssi"), "%d", Wifi1.getRSSI());
//      publish(F("info/df"), CxSpiffs::df());
//      publish(F("info/freeota"), CxTools::getFreeOTA());

   }
}

#endif /*ESP_CONSOLE_NOFS*/
