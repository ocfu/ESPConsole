//
//  CxESPConsoleFS.hpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPConsoleFS_hpp
#define CxESPConsoleFS_hpp

#include "CxESPConsole.hpp"
#include "CxESPConsoleExt.hpp"

#ifndef ESP_CONSOLE_NOFS
#ifdef ARDUINO
#include <FS.h>
#ifdef ESP32
#include "LITTLEFS.h"
struct FSInfo {
   size_t totalBytes;
   size_t usedBytes;
   size_t blockSize;
   size_t pageSize;
   size_t maxOpenFiles;
   size_t maxPathLength;
};
#define Dir File
#define LittleFS LITTLEFS
#else  // no ESP32
#include <LittleFS.h>
#endif // end ESP32
#endif // end ARDUINO

class CxESPConsoleFS : public CxESPConsoleExt {
private:
   
   void _getFSInfo(FSInfo& fsinfo) {
#ifdef ARDUINO
#ifdef ESP32
      fsinfo.totalBytes = LITTLEFS.totalBytes();
      fsinfo.usedBytes = LITTLEFS.usedBytes();
#else
      LittleFS.info(fsinfo);
#endif
#endif
   }
   
#ifndef ESP_CONSOLE_NOWIFI
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const override {
      return new CxESPConsoleFS(wifiClient, app, ver);
   }
#endif
   
protected:
   virtual bool __processCommand(const char* szCmd, bool bQuiet = false) override;

   void __printNoFS() {
      println(F("file system not mounted!"));
   }

   void __printNoSuchFileOrDir(const char* szCmd, const char* szFn = nullptr) {
      if (szCmd && szFn) printf(F("%s: %s: No such file or directory\n"), szCmd, szFn);
      if (szCmd && !szFn) printf(F("%s: null : No such file or directory\n"), szCmd);
   };
   
public:
#ifndef ESP_CONSOLE_NOWIFI
   CxESPConsoleFS(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsoleFS((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
#endif
   CxESPConsoleFS(Stream& stream, const char* app = "", const char* ver = "") : CxESPConsoleExt(stream, app, ver){}


   using CxESPConsole::begin;
   virtual void begin() override;
   // specifics for this console class, when needed
   //virtual void end() override {CxESPConsoleExt::end();}
   //virtual void loop() override {CxESPConsoleExt::loop();}
   virtual bool hasFS() override;
   
   virtual void printInfo() override;


   void printFsUsage();
   //void printFileDateTime(time_t cr, time_t lw);

   void printDu(bool fmt = false, const char* szFn = nullptr);
   void printSize(bool fmt = false);
   void printDf(bool fmt = false);
   void ls(bool all = false, bool bLong = false);
   void cat(const char* szFn);
   void rm(const char* szFn);
   void cp(const char* src, const char* dst);
   void touch(const char* szFn);
   void mount();
   void umount();
   void format();
   
   void saveEnv(const char* szEnv, const char* szValue);
   bool loadEnv(const char* szEnv, String& strValue);


};

#endif /*ESP_CONSOLE_NOFS*/

#endif /* CxESPConsole_hpp */
