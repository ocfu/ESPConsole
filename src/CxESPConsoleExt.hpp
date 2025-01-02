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

#ifdef ARDUINO
#ifdef ESP32
#else  // no ESP32
#endif // end ESP32
#endif // end ARDUINO

class CxESPConsoleExt : public CxESPConsole {
private:
   String _strCoreSdkVersion;
   
#ifndef ESP_CONSOLE_NOWIFI
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleExt(wifiClient, app, ver);
   }
#endif
   
protected:
   virtual bool __processCommand(const char* szCmd, bool bQuiet = false) override;

public:
#ifndef ESP_CONSOLE_NOWIFI
   CxESPConsoleExt(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsoleExt((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
#endif

   CxESPConsoleExt(Stream& stream, const char* app = "", const char* ver = "") : CxESPConsole(stream, app, ver){
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
   //virtual void loop() override {CxESPConsoleExt::loop();}

   void printHW();
   void printSW();
   void printESP();
   void printFlashMap();
   
   void printNetworkInfo();

   
   virtual void printInfo() override;


};

#endif /* CxESPConsoleExt_hpp */
