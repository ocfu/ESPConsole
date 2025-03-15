/**
 * @file CxCapabilityMqttHA.hpp
 * @brief MQTT Home Assistant Capability for ESP-based projects
 *
 * This file defines the `CxCapabilityMqttHA` class, which provides MQTT Home Assistant
 * capabilities for an ESP-based project. It includes methods for setting up, managing,
 * and executing MQTT Home Assistant-related commands.
 *
 * @date 09.01.25
 * @author ocfu
 * @copyright © 2025 ocfu
 */
#ifndef CxCapabilityMqttHA_hpp
#define CxCapabilityMqttHA_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityMqtt.hpp"

#include "../tools/CxMqttHAManager.hpp"

class CxCapabilityMqttHA : public CxCapability {
   CxESPConsoleMaster& console = CxESPConsoleMaster::getInstance();
   CxMqttHADevice& _mqttHAdev = CxMqttHADevice::getInstance();
   CxMqttManager& __mqttManager = CxMqttManager::getInstance();

   bool _bHAEnabled = false;
   
   CxTimer60s _timer60s;
   
   std::map<uint8_t, std::unique_ptr<CxMqttHADiagnostic>> _mapHADiag;

public:
   explicit CxCapabilityMqttHA()
   : CxCapability("mqttha", getCmds()) {}
   static constexpr const char* getName() { return "mqttha"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "ha" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityMqttHA>();
   }
   
   ~CxCapabilityMqttHA() {
      enableHA(false);
      _mapHADiag.clear();
   }
   
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*console.getStream());
      __bLocked = false;
      
      console.info(F("====  Cap: %s  ===="), getName());
      
      // increase the PubSubClient buffer size as for HA the payload could be pretty long, especially for discovery topics.
      __mqttManager.setBufferSize(1024);
      
      _mapHADiag[0] = std::make_unique<CxMqttHADiagnostic>("Status", "diagstatus", true, __mqttManager.getRootPath());
      _mapHADiag[1] = std::make_unique<CxMqttHADiagnostic>("Reconnects/h", "diagreconnects", nullptr, "1/h");
      _mapHADiag[2] = std::make_unique<CxMqttHADiagnostic>("Linkquality", "diaglink", nullptr, "rssi");
      _mapHADiag[3] = std::make_unique<CxMqttHADiagnostic>("Free mem", "diagmem", nullptr, "bytes");
      _mapHADiag[4] = std::make_unique<CxMqttHADiagnostic>("Last Restart", "diagrestart", "timestamp", "", true);
      _mapHADiag[5] = std::make_unique<CxMqttHADiagnostic>("Up Time", "diaguptime", "duration", "s");
      _mapHADiag[6] = std::make_unique<CxMqttHADiagnostic>("Restart Reason", "diagreason", nullptr, nullptr, true);
      _mapHADiag[7] = std::make_unique<CxMqttHADiagnostic>("Stack", "diagstack", nullptr, nullptr, true);
      _mapHADiag[8] = std::make_unique<CxMqttHADiagnostic>("Stack Low", "diagstacklow", nullptr, nullptr, true);

      execute ("ha load");
      
      if (isEnabled()) enableHA(true);

   }
   
   void loop() override {
      if (_timer60s.isDue()) {
         if (_mapHADiag[0]) {
            StaticJsonDocument<256> doc;
            doc[F("heartbeat")] = millis();
            doc[F("uptime")] = console.getUpTimeISO();
            doc[F("connects")] = __mqttManager.getConnectCntr();
#ifdef ARDUINO
            doc[F("RSSI")] = WiFi.RSSI();
#endif
            _mapHADiag[0]->publishAttributes(doc);
         }
         
         if (_mapHADiag[1]) {
            static uint32_t n1hBucket = 0;
            static uint32_t nLastCounter = 0;
            static unsigned long nTimer1h = 0;
            
            n1hBucket = (uint32_t)fmax(n1hBucket, __mqttManager.getConnectCntr() - nLastCounter);
            
            _mapHADiag[1]->publishAvailability(true);
            
            if ((millis() - nTimer1h) > (3600000)) {
               _mapHADiag[1]->publishState(n1hBucket, 0);
               nTimer1h = millis();
               n1hBucket = 0;
               nLastCounter = __mqttManager.getConnectCntr();
            }
         }
#ifdef ARDUINO
         if (_mapHADiag[2]) _mapHADiag[2]->publishState(WiFi.RSSI(), 0);
         if (_mapHADiag[3]) _mapHADiag[3]->publishState(ESP.getFreeHeap(), 0);
#endif
         if (_mapHADiag[5]) _mapHADiag[5]->publishState(console.getUpTimeSeconds(), 0);
         if (_mapHADiag[7]) _mapHADiag[7]->publishState(g_Stack.getSize(), 0);
         if (_mapHADiag[8]) _mapHADiag[8]->publishState(g_Stack.getLow(), 0);

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
      }    if (cmd == "ha") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         String strEnv = ".ha";
         if (strSubCmd == "enable") {
            _bHAEnabled = (bool)TKTOINT(tkArgs, 2, 0);
            enableHA(_bHAEnabled);
         } else if (strSubCmd == "list") {
            _mqttHAdev.printList(getIoStream());
         } else if (strSubCmd == "save") {
            CxConfigParser Config;
            Config.addVariable("enabled", _bHAEnabled);
            ESPConsole.saveEnv(strEnv, Config.getConfigStr());
         } else if (strSubCmd == "load") {
            String strValue;
            if (ESPConsole.loadEnv(strEnv, strValue)) {
               CxConfigParser Config(strValue.c_str());
               // extract settings and set, if defined. Keep unchanged, if not set.
               _bHAEnabled = Config.getBool("enabled", _bHAEnabled);
               console.info(F("Mqtt HA support enabled: %d"), _bHAEnabled);
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
      g_Stack.update();
      return true;
   }
      
   bool isEnabled() {return _bHAEnabled;}
   void setEnabled(bool set) {_bHAEnabled = set;}
   
   void enableHA(bool enabled) {
      _mqttHAdev.setFriendlyName(console.getAppName());
      _mqttHAdev.setName(console.getAppName());
      _mqttHAdev.setTopicBase("ha");
      _mqttHAdev.setManufacturer("ocfu");
      _mqttHAdev.setModel("my Model");
      _mqttHAdev.setSwVersion(console.getAppVer());
      _mqttHAdev.setHwVersion("ESP");
      _mqttHAdev.setUrl("");
      _mqttHAdev.setStrId();
      
      _mqttHAdev.regItems(enabled);
      _mqttHAdev.publishAvailability(enabled);
      
      if (enabled) {
         if (_mapHADiag[6]) _mapHADiag[6]->publishState(::getResetInfo());
         if (_mapHADiag[4]) _mapHADiag[4]->publishState(console.getStartTime());
      }
   }
};

#endif /* CxCapabilityMqttHA_hpp */
