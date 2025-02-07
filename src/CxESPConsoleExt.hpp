//
//  CxESPConsoleExt.hpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPConsoleExt_hpp
#define CxESPConsoleExt_hpp

#include "CxESPConsole.hpp"
#include "CxLed.hpp"

#ifdef ARDUINO
#ifdef ESP32
#else  // no ESP32
#endif // end ESP32
#endif // end ARDUINO

#ifndef LED_BUILTIN
#define LED_BUILTIN -1
#endif


class CxESPConsoleExt : public CxESPConsole {
private:
   String _strCoreSdkVersion;
   CxGPIOTracker& _gpioTracker = CxGPIOTracker::getInstance();

   
#ifndef ESP_CONSOLE_NOWIFI
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleExt(wifiClient, app, ver);
   }
#endif
   
   bool _processCommand(const char* szCmd, bool bQuiet = false);

   static void _handleRoot();
   static void _handleConnect();
   void _beginAP();
   void _stopAP();
   
protected:
   
#ifndef ESP_CONSOLE_NOWIFI
   virtual void __prompt() override {
      if (__inAPMode()) {
         print(ESC_CLEAR_LINE);
         printf(FMT_PROMPT_USER_HOST_APMODE);
      } else if(!isConnected()) {
         print(ESC_CLEAR_LINE);
         printf(FMT_PROMPT_USER_HOST_OFFLINE);
      } else {
         CxESPConsole::__prompt();
      }
   }
#ifdef ARDUINO
   bool __inAPMode() {return (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);}
#else
   bool __inAPMode() {return false;}
#endif
#endif

public:
#ifndef ESP_CONSOLE_NOWIFI
   CxESPConsoleExt(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsoleExt((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
#endif
 
   CxESPConsoleExt(Stream& stream, const char* app = "", const char* ver = "") : Led1(LED_BUILTIN), CxESPConsole(stream, app, ver) {

      // register commmand set for this class
      commandHandler.registerCommandSet(F("Extended"), [this](const char* cmd, bool bQuiet)->bool {return _processCommand(cmd, bQuiet);}, F("hw, sw, net, esp, flash, net, set, eeprom, wifi, gpio, led"), F("Extended commands"));

#ifdef ARDUINO
      _strCoreSdkVersion = "core ";
      _strCoreSdkVersion += ESP.getCoreVersion();
      _strCoreSdkVersion += " sdk ";
      _strCoreSdkVersion += ESP.getSdkVersion();
#endif
   }
   

   ///
   /// Make "void begin(WiFiServer& server)" visible from base class to enable the
   /// call of begin() (the top inherited one) as well as begin(server) (from the base class).
   /// The later will handle connections on the WiFi server in the base class loop().
   ///
   using CxESPConsole::begin;

   ///
   /// These inherited functions will be call from top to base.
   ///
   virtual void begin() override;
   // specifics for this console class, when needed
   //virtual void end() override {CxESPConsoleExt::end();}
   virtual void loop() override;

#ifndef ESP_CONSOLE_NOWIFI
   bool isHostAvailble(const char* server, uint32_t port);
   void startWiFi(const char* ssid = nullptr, const char* pw = nullptr);
   void stopWiFi();
   
   void readSSID(String& strSSID);
   void readPassword(String& strPassword);
   void readHostName(String& strHostName);
   void readOtaPassword(String& strPassword);
#endif
   
   void printHW();
   void printSW();
   void printESP();
   void printFlashMap();
   void printEEProm(uint32_t nStartAddr = 0, uint32_t nLength = 512);
   
   void printNetworkInfo();

   
   virtual void printInfo() override;
   
   CxLed Led1;

   void ledAction();

};

#endif /* CxESPConsoleExt_hpp */
