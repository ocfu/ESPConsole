//
//  CxESPConsoleLog.hpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPConsoleLog_hpp
#define CxESPConsoleLog_hpp

#include "CxESPConsoleFS.hpp"

#ifndef ESP_CONSOLE_NOFS
#ifdef ARDUINO
#ifdef ESP32
#else  // no ESP32
#endif // end ESP32
#endif // end ARDUINO

class CxESPConsoleLog : public CxESPConsoleFS {
private:
         
   String _strLogServer = "";
   uint32_t _nLogPort = 0;
   bool _bLogServerAvailable = false;
   uint32_t _nLastLogServerCheck = 0;
   
#ifndef ESP_CONSOLE_NOWIFI
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleLog(wifiClient, app, ver);
   }
#endif
   
   void _print2logServer(const char* sz);

protected:
   virtual bool __processCommand(const char* szCmd, bool bQuiet = false) override;

public:
#ifndef ESP_CONSOLE_NOWIFI
   CxESPConsoleLog(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsoleLog((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
#endif
   CxESPConsoleLog(Stream& stream, const char* app = "", const char* ver = "") : CxESPConsoleFS(stream, app, ver){}

   using CxESPConsole::begin;
   virtual void begin() override;
   // specifics for this console class, when needed
   //virtual void end() override {CxESPConsoleFS::end();}
   //virtual void loop() override {CxESPConsoleFS::loop();}
   
   virtual void printInfo() override;
   
   virtual void __debug(const char* sz) override;
   virtual void __debug_ext(uint32_t flang, const char* sz) override;
   virtual void __info(const char* sz) override;
   virtual void __warn(const char* sz) override;
   virtual void __error(const char* sz) override;
   
};

#endif /*ESP_CONSOLE_NOFS*/

#endif /* CxESPConsoleLog_hpp */
