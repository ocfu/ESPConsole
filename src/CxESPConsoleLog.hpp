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

#ifdef DEBUG_BUILD
#  define _LOG_DEBUG(...) debug(__VA_ARGS__)
#  define _LOG_DEBUG_EXT(...) debug_ext(__VA_ARGS__)
#else
#  define _LOG_DEBUG(...) ((void)0)
#  define _LOG_DEBUG_EXT(...) ((void)0)
#endif


#define LOGLEVEL_OFF       0
#define LOGLEVEL_ERROR     1
#define LOGLEVEL_WARN      2
#define LOGLEVEL_INFO      3
#define LOGLEVEL_DEBUG     4
#define LOGLEVEL_DEBUG_EXT 5

class CxESPConsoleLog : public CxESPConsoleFS {
private:
      
   ///
   /// log levels
   ///  0: off
   ///  1: error
   ///  2: warning
   ///  3: info
   ///  4: debug
   ///  5: ext. debug (controlled by _nDebugFlag)
   ///
   uint32_t _nLogLevel  = 0;
   
   ///
   /// debug flags 32 bits 0xfffffffe (except: -1)
   /// 0x0: off
   ///
   ///
   uint32_t _nExtDebugFlag = 0x0;
   
#ifndef ESP_CONSOLE_NOWIFI
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleLog(wifiClient, app, ver);
   }
#endif
   
   void _printPrefix(char c);

   void debug(const char* sz...);
   void debug(String& str) {debug(str.c_str());}
   void debug(const FLASHSTRINGHELPER * szP...);

   
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
   
};

#endif /*ESP_CONSOLE_NOFS*/

#endif /* CxESPConsoleLog_hpp */
