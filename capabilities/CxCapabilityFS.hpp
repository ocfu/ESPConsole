/**
 * @file CxCapabilityFS.hpp
 * @brief This file defines the CxCapabilityFS class for handling file system operations on an ESP-based system using LittleFS.
 *
 * @author ocfu
 * @date 09.01.25
 * @copyright © 2025 ocfu. All rights reserved.
 *
 * The CxCapabilityFS class provides various file system operations such as mounting, unmounting, formatting, and performing file operations like ls, cat, cp, rm, and touch.
 * It also includes methods for handling environment variables and logging.
 */

#ifndef CxCapabilityFS_hpp
#define CxCapabilityFS_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"
#include "ArduinoJson.h"


#include "../capabilities/CxCapabilityExt.hpp"

#include "../tools/CxConfigParser.hpp"
#include "esphw.h"

#ifndef ESP_CONSOLE_NOWIFI
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
#else
#include <LittleFS.h>
#endif /* ESP32*/
#endif /* ARDUINO */
#endif /* ESP_CONSOLE_NOWIFI */

class CxCapabilityFS : public CxCapability {
   
   CxESPConsoleMaster& console = CxESPConsoleMaster::getInstance();
   
   String _strLogServer = "";
   uint32_t _nLogPort = 0;
   bool _bLogServerAvailable = false;
   
   CxTimer60s _timer60sLogServer;


public:

   explicit CxCapabilityFS() : CxCapability("fs", getCmds()) {}
   static constexpr const char* getName() { return "fs"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "du", "df", "size", "ls", "cat", "cp", "rm", "touch", "mount", "umount", "format", "fs", "save", "load", "log" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityFS>();
   }
   
   ~CxCapabilityFS() {
      umount();
      
      // remove log functions
      ESPConsole.setFuncDebug([this](const char *c) { this->_debug(c); });
      ESPConsole.setFuncDebugExt([this](uint32_t flag, const char *c) { this->_debug_ext(flag, c); });
      ESPConsole.setFuncInfo([this](const char *c) { this->_info(c); });
      ESPConsole.setFuncWarn([this](const char *c) { this->_warn(c); });
      ESPConsole.setFuncError([this](const char *c) { this->_error(c); });
   }
   
   void setup() override {
      CxCapability::setup();
      
      __bLocked = false;
      
      console.info(F("====  Cap: %s  ===="), getName());

      // load specific environments for this class
      mount();
      
      execute("load ntp");
      execute("load tz");
      execute("load led");
      execute ("log load");
      
      // implement log functions
      ESPConsole.setFuncDebug([this](const char *c) { this->_debug(c); });
      ESPConsole.setFuncDebugExt([this](uint32_t flag, const char *c) { this->_debug_ext(flag, c); });
      ESPConsole.setFuncInfo([this](const char *c) { this->_info(c); });
      ESPConsole.setFuncWarn([this](const char *c) { this->_warn(c); });
      ESPConsole.setFuncError([this](const char *c) { this->_error(c); });
      
      ESPConsole.setFuncLoadEnv([this](String &strEnv, String &strValue)->bool { return this->loadEnv(strEnv, strValue); });
      ESPConsole.setFuncSaveEnv([this](String &strEnv, String &strValue) {this->saveEnv(strEnv, strValue);});


   }
   
   void loop() override {
   }
   
   bool execute(const char *szCmd) override {
       
      // validate the call
      if (!szCmd) return false;
      
      // get the command and arguments into the token buffer
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
       } else if (cmd == "du") {printDu(a);a ? println() : println(" .");
       } else if (cmd == "df") {printDf();println(F(" bytes"));
       } else if (cmd == "size") {printSize();println(F(" bytes"));
       } else if (cmd == "ls") {
          String strOpt = TKTOCHAR(tkArgs, 1);
          ls(strOpt == "-a" || strOpt == "-la", strOpt == "-l" || strOpt == "-la");
       } else if (cmd == "cat") {cat(a);
       } else if (cmd == "cp") {cp(a, b);
       } else if (cmd == "rm") {rm(a);
       } else if (cmd == "touch") {
          touch(a);
       } else if (cmd == "mount") {
          mount();
       } else if (cmd == "umount") {
          umount();
       } else if (cmd == "format") {
          format();
       } else if (cmd == "hasfs") {
          return hasFS();
       }
       else if (cmd == "fs") {
          printFsInfo();
          println();
       } else if (cmd == "save") {
          ///
          /// known env variables:
          /// - ntp
          /// - tz
          ///
          /// environment variables are stored in files. the file name is the name of the variable, with a trailing '.',
          /// which indicates that it is a hidden file. the content of the file is the value of the variable.
          ///
          
          String strEnv = ".";
          strEnv += TKTOCHAR(tkArgs, 1);
          String strValue;
          if (strEnv == ".ntp") {
             strValue = console.getNtpServer();
             saveEnv(strEnv, strValue);
          } else if (strEnv == ".tz") {
             strValue = console.getTimeZone();
             saveEnv(strEnv, strValue);
          } else if (strEnv == ".mqtt") {
             strValue = cmd.substring(5);
             strValue.trim();
             saveEnv(strEnv, strValue);
          } else {
             println(F("save environment variable. \nusage: save <env>"));
             println(F("known env variables:\n ntp \n tz "));
             println(F("example: save ntp"));
          }
       } else if (cmd == "load") {
          String strEnv = ".";
          strEnv += TKTOCHAR(tkArgs, 1);
          String strValue;
          if (strEnv == ".ntp") {
             if (loadEnv(strEnv, strValue)) {
                console.setNtpServer(strValue.c_str());
                console.info(F("NTP server set to %s"), console.getNtpServer());
             } else {
                console.warn(F("NTP server env variable (ntp) not found!"));
             }
          } else if (strEnv == ".tz") {
             if (loadEnv(strEnv, strValue)) {
                console.setTimeZone(strValue.c_str());
                console.info(F("Timezone set to %s"), console.getTimeZone());
             } else {
                console.warn(F("Timezone env variable (tz) not found!"));
             }
          } else {
             println(F("load environment variable.\nusage: load <env>"));
             println(F("known env variables:\n ntp \n tz "));
             println(F("example: load ntp"));
          }
       } else if (cmd == "$UPLOAD$") {
          _handleFile();
       } else if (cmd == "$DOWNLOAD$") {
          _handleFile();
       } else if (cmd == "log") {
          String strSubCmd = TKTOCHAR(tkArgs, 1);
          String strEnv = ".log";
          if (strSubCmd == "server") {
             _strLogServer = TKTOCHAR(tkArgs, 2);
             _bLogServerAvailable = console.isHostAvailable(_strLogServer.c_str(), _nLogPort);
             if (!_bLogServerAvailable) println(F("server not available!"));
          } else if (strSubCmd == "port") {
             _nLogPort = TKTOINT(tkArgs, 2, 0);
             _bLogServerAvailable = console.isHostAvailable(_strLogServer.c_str(), _nLogPort);
             if (!_bLogServerAvailable) println(F("server not available!"));
          } else if (strSubCmd == "level") {
             console.setLogLevel(TKTOINT(tkArgs, 2, console.getLogLevel()));
          } else if (strSubCmd == "save") {
             CxConfigParser Config;
             Config.addVariable("level", console.getLogLevel());
             Config.addVariable("server", _strLogServer);
             Config.addVariable("port", _nLogPort);
             saveEnv(strEnv, Config.getConfigStr());
          } else if (strSubCmd == "load") {
             String strValue;
             if (loadEnv(strEnv, strValue)) {
                CxConfigParser Config(strValue);
                // extract settings and set, if defined. Keep unchanged, if not set.
                console.setLogLevel(Config.getInt("level", console.getLogLevel()));
                _strLogServer = Config.getSz("server", _strLogServer.c_str());
                _nLogPort = Config.getInt("port", _nLogPort);
                if (_strLogServer.length() && _nLogPort > 0) {
                   _bLogServerAvailable = true;
                   _timer60sLogServer.makeDue();
                }
             }
          } else if (strSubCmd == "error") {
             _error(a);
          } else if (strSubCmd == "info") {
             _info(a);
          } else if (strSubCmd == "warn") {
             _warn(a);
          } else if (strSubCmd == "debug") {
             _debug(a);
          } else if (strSubCmd == "debug_ext") {
             _debug_ext(TKTOINT(tkArgs, 3, 0), a);
          } else {
             printf(F(ESC_ATTR_BOLD "Log level:       " ESC_ATTR_RESET "%d"), console.getLogLevel());printf(F(ESC_ATTR_BOLD " Usr: " ESC_ATTR_RESET "%d\n"), console.getUsrLogLevel());
             printf(F(ESC_ATTR_BOLD "Ext. debug flag: " ESC_ATTR_RESET "0x%X\n"), console.getDebugFlag());
             printf(F(ESC_ATTR_BOLD "Log server:      " ESC_ATTR_RESET "%s (%s)\n"), _strLogServer.c_str(), _bLogServerAvailable?"online":"offline");
             printf(F(ESC_ATTR_BOLD "Log port:        " ESC_ATTR_RESET "%d\n"), _nLogPort);
             println(F("log commands:"));
             println(F("  server <server>"));
             println(F("  port <port>"));
             println(F("  level <level>"));
             println(F("  save"));
             println(F("  load"));
             console.info(F("test log message"));
          }
       } else {
          return false;
       }
      g_Stack.update();
      return true;
   }

   bool hasFS() {
      bool bResult = false;
#ifdef ARDUINO
      FSInfo fsinfo;
#ifdef ESP32
      fsinfo.totalBytes = LittleFS.totalBytes();
      bResult = (fsinfo.totalBytes > 0);
#else
      bResult = LittleFS.info(fsinfo);
#endif
#endif
      return bResult;
   }

   uint32_t getDf() {
      if (hasFS()) {
         FSInfo fsinfo;
         _getFSInfo(fsinfo);
         return (uint32_t) (fsinfo.totalBytes - fsinfo.usedBytes);
      } else {
         return 0;
      }
   }
   
   void printFsInfo() {
      if (hasFS()) {
         print(F(ESC_ATTR_BOLD   "Filesystem: " ESC_ATTR_RESET "Little FS"));
         print(F(ESC_ATTR_BOLD " Size: " ESC_ATTR_RESET));printSize();print(F(" bytes"));
         print(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));printDu();print(F(" bytes"));
         print(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));printDf();print(F(" bytes"));
      } else {
         print(F(ESC_ATTR_BOLD "Filesystem: " ESC_ATTR_RESET "not mounted"));
      }
   }
   
   void printDu(bool fmt = false, const char* szFn = nullptr) {
      if (hasFS()) {
         if (szFn) {
#ifdef ARDUINO
            if (LittleFS.exists(szFn)) {
               File file = LittleFS.open(szFn, "r");
               if (file) {
                  if (fmt) {
                     printf(F("%07d %s"), file.size(), file.name());
                  } else {
                     printf(F("%d %s"), file.size(), file.name());
                  }
               }
            } else {
               _printNoSuchFileOrDir("du", szFn);
            }
#endif
            
         } else {
            FSInfo fsinfo;
            _getFSInfo(fsinfo);
            if (fmt) {
               printf(F("%7ld"), fsinfo.usedBytes);
            } else {
               printf(F("%ld"), fsinfo.usedBytes);
            }
         }
      } else {
         _printNoFS();
      }
   }
   
   void printSize(bool fmt = false) {
      if (hasFS()) {
         FSInfo fsinfo;
         _getFSInfo(fsinfo);
         if (fmt) {
            printf(F("%07ld"), fsinfo.totalBytes);
         } else {
            printf(F("%ld"), fsinfo.totalBytes);
         }
      } else {
         _printNoFS();
      }
   }

   void printDf(bool fmt = false)  {
      if (hasFS()) {
         if (fmt) {
            printf(F("%7ld"), getDf());
         } else {
            printf(F("%ld"), getDf());
         }
      } else {
         _printNoFS();
      }
   }

   void ls(bool bAll = false, bool bLong = false) {
      if (hasFS()) {
         uint32_t totalBytes;
         uint32_t usedBytes;
         
         FSInfo fsinfo;
         _getFSInfo(fsinfo);
         
         totalBytes = (uint32_t) fsinfo.totalBytes;
         usedBytes =  (uint32_t) fsinfo.usedBytes;
         
#ifdef ARDUINO
         uint32_t total = 0;
#ifdef ESP32
         // TODO: recursively https://unsinnsbasis.de/littlefs-esp32/
         File root = LittleFS.open("/");
         if (root) {
            File file = root.openNextFile();
            while (file) {
               if (file.isDirectory()) {
                  printf(F("DIR     %s/\n"), file.name());
               } else {
                  const char* fn = file.name();
                  
                  // skip hidden files
                  if (!bAll && fn && fn[0] == '.') continue;
                  
                  if (bAll) {
                     printf(F("%7d "), file.size());
                     console.printFileDateTime(getIoStream(), file.getCreationTime(), file.getLastWrite());
                  }
                  printf(F(" %s\n"), file.name());
               }
               file = root.openNextFile();
            }
         }
#else // no ESP32
         Dir dir = LittleFS.openDir("");
         while (dir.next()) {
            File file = dir.openFile("r");
            const char* fn = file.name();
            
            // skip hidden files
            if (!bAll && fn && fn[0] == '.') continue;
            
            // print file size and date/time
            if (bLong) {
               printf(F("%7d "), file.size());
               console.printFileDateTime(getIoStream(), file.getCreationTime(), file.getLastWrite());
            }
            printf(F(" %s\n"), file.name());
            total += file.size();
            file.close();
         }
#endif // end ESP32
         if (bLong) {
            printf(F("%7d (%d bytes free)\n"), total, totalBytes - usedBytes);
         }
#endif // end ARDUINO
      } else {
         _printNoFS();
      }
   }
   
   void cat(const char* szFn) {
      if (! szFn) {
         println(F("usage: cat <file>"));
         return;
      }
      if (hasFS()) {
#ifdef ARDUINO
         // Open file for reading
         File file = LittleFS.open(szFn, "r");
         if (file) {
            while (file.available()) {
               print((char)file.read());
            }
            println();
         } else {
            _printNoSuchFileOrDir("cat", szFn);
            return;
         }
#else
         std::ifstream file;
         file.open(szFn);
         if (file.is_open()) {
            char c = 0;
            while (file.get(c)) {
               print(c);
            }
            println();
         } else {
            return;
         }
#endif
         file.close();
      } else {
         _printNoFS();
      }
   }
   
   void rm(const char* szFn) {
      if (! szFn) {
         println(F("usage: rm <file>"));
         return;
      }
      if (hasFS()) {
#ifdef ARDUINO
         if (!LittleFS.remove(szFn)) {
            _printNoSuchFileOrDir("rm", szFn);
         }
#else
#endif
      } else {
         _printNoFS();
      }
   }
   
   void cp(const char *szSrc, const char *szDst) {
      if (! szSrc || ! szDst) {
         println(F("usage: cp <src_file> <tgt_file>"));
         return;
      }
      if (hasFS()) {
#ifdef ARDUINO
         if (LittleFS.exists(szSrc)) {
            static char buf[64];
            
            // FIXME: cp need y/n query if dst exist, unless -f is given as parameter
            if (LittleFS.exists(szDst)) LittleFS.remove(szDst);
            
            File fileSrc = LittleFS.open(szSrc, "r");
            
            if (fileSrc) {
               File fileDst = LittleFS.open(szDst, "w");
               if (fileDst) {
                  while (fileSrc.available() > 0) {
                     uint8_t n = fileSrc.readBytes(buf, sizeof(buf));
                     fileDst.write((uint8_t*)buf, n);
                  }
                  fileDst.close();
               }
               fileSrc.close();
            }
         } else {
            _printNoSuchFileOrDir("cp", szSrc);
         }
#else
#endif
      } else {
         _printNoFS();
      }
   }
   
   void touch(const char* szFn) {
      if (! szFn) {
         println(F("usage: touch <file>"));
         return;
      }
      if (hasFS()) {
#ifdef ARDUINO
         const char* mode = "a";
         if (!LittleFS.exists(szFn)) {
            mode = "w";
         }
         File file = LittleFS.open(szFn, "w");
         file.close();
#else
#endif
      } else {
         _printNoFS();
      }
   }
   
   void mount() {
      if (!hasFS()) {
#ifdef ARDUINO
         if (!LittleFS.begin()) {
            console.error("LittleFS mount failed");
            return;
         }
#else
#endif
      } else {
         //println(F("LittleFS already mounted!"));
      }
   }
   
   void umount() {
      if (hasFS()) {
#ifdef ARDUINO
         LittleFS.end();
#else
#endif
      }
   }
   
   void format() {
      if (hasFS()) {
         println(F("LittleFS still mounted! -> 'umount' first"));
      } else {
//         console.promptUserYN("Are you sure you want to format?", [](bool confirmed) {
//            if (confirmed) {
#ifdef ARDUINO
               LittleFS.format();
#endif
//            }
//         });
//#else
//#endif
      }
   }
   
   void saveEnv(String& strEnv, String& strValue) {
      if (hasFS()) {
         console.debug(F("save env variable %s, value=%s"), strEnv.c_str(), strValue.c_str());
#ifdef ARDUINO
         File file = LittleFS.open(strEnv.c_str(), "w");
         if (file) {
            file.print(strValue.c_str());
            file.close();
         }
#endif
      } else {
         println(F("file system not mounted!"));
      }
   }
   
   bool loadEnv(String& strEnv, String& strValue) {
      if (hasFS()) {
         console.debug(F("load env variable %s"), strEnv.c_str());
#ifdef ARDUINO
         File file = LittleFS.open(strEnv.c_str(), "r");
         if (file) {
            while (file.available()) {
               strValue += (char)file.read();
            }
            file.close();
            return true;
         }
#endif
      } else {
         println(F("file system not mounted!"));
      }
      return false;
   }

   
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
   
   bool _handleFile() {
#ifdef ARDUINO
      static char buffer[64];
      String header = "";
      String filename = "";
      size_t expectedSize = 0;
      size_t receivedSize = 0;
      File file;
      bool bError = false;
      
      WiFiClient* client = static_cast<WiFiClient*>(&getIoStream()); // tricky
      
      // read header
      while (client->connected() && header.indexOf("\n") == -1) {
         if (client->available()) {
            char c = client->read();
            header += c;
         }
      }
      
      //console.debug(F("receive header: %s"), header.c_str());
      
      // analyse header
      if (header.startsWith("GET ")) {
         // Handle GET <filename>
         String filename = header.substring(4);
         filename.trim();
         return _sendFile(client, filename.c_str());
      } else if (header.startsWith("FILE:")) {
         // receive file
         int fileStart = header.indexOf("FILE:") + 5;
         int sizeStart = header.indexOf("SIZE:") + 5;
         int sizeEnd = header.indexOf("\n");
         
         filename = header.substring(fileStart, header.indexOf(" ", fileStart));
         expectedSize = header.substring(sizeStart, sizeEnd).toInt();
         
         if (expectedSize > getDf() * 0.9) {
            println(F("not enough space available for the file!"));
            console.error(F("not enough space available for the file!"));
            return false;
         }
         
         console.info(F("receive file: %s (size: %d Bytes)"), filename.c_str(), expectedSize);
         
         // file open
         file = LittleFS.open(filename, "w");
         if (!file) {
            println(F("error: create file"));
            console.error(F("error: create file %s"), filename.c_str());
            return false;
         }
      } else {
         println(F("error: invalid header"));
         console.error(F("error: invalid header received during file transfer"));
         return false;
      }
      
      // receive file data
      CxTimer timerTO(5000); // set timeout
      
      while (client->connected() && receivedSize < expectedSize) {
         size_t bytesToRead = client->available();
         if (bytesToRead > 0) {
            size_t bytesRead = client->readBytes(buffer, min(bytesToRead, sizeof(buffer)));
            file.write((uint8_t *)buffer, bytesRead);
            receivedSize += bytesRead;
            //printProgress(receivedSize, expectedSize, filename.c_str(), "bytes");
            console.printProgressBar((uint32_t)receivedSize, (uint32_t)expectedSize, filename.c_str());
            timerTO.restart(); // reset timeout
         } else if (timerTO.isDue()) { // timeout
            console.error(F("timeout receiving a file"));
            bError = true;
            break;
         }
         delay(1);
      }
      println(" done!");
      file.close();
      
      // Empfang überprüfen
      if (receivedSize == expectedSize) {
         console.info(F("file transfer finished."));
      } else {
         printf(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "Warning: received size of data (%d bytes) not same as expected file size (%d bytes) !\n" ESC_ATTR_RESET), receivedSize, expectedSize);
         console.error(F("received size of data (%d bytes) not same as expected file size (%d bytes)!"), receivedSize, expectedSize);
      }
#endif
      return true;
   }
   
   bool _sendFile(WiFiClient* client, const char* filename) {
#ifdef ARDUINO
      // without the log capability, it would print to __ioStream, which is the client stream for the file transfer!
      //console.debug(F("download file: %s"), filename);
      
      File file = LittleFS.open(filename, "r");
      if (!file) {
         client->println("ERROR: File not found");
         //console.warn("File not found: %s", filename);
         return false;
      }
      
      size_t fileSize = file.size();
      client->printf("SIZE: %d\n", fileSize);
      //console.info("Sending file: %s (%d bytes)\n", filename, fileSize);

      static char buffer[64];
      size_t bytesRead;

      g_Stack.update();

      // Send file data
      while ((bytesRead = file.readBytes(buffer, sizeof(buffer))) > 0) {
         client->write((uint8_t *)buffer, bytesRead);
      }
      
      file.close();
      //console.info("File transfer complete.");
      
#endif
      return true;
   }
   
   
   
   void _printNoFS() {
      println(F("file system not mounted!"));
   }
   
   void _printNoSuchFileOrDir(const char* szCmd, const char* szFn = nullptr) {
      if (szCmd && szFn) printf(F("%s: %s: No such file or directory\n"), szCmd, szFn);
      if (szCmd && !szFn) printf(F("%s: null : No such file or directory\n"), szCmd);
   };

   void _print2logServer(const char* sz) {
      if (!_strLogServer.length() || _nLogPort < 1) return;
      
      bool bAvailable = _bLogServerAvailable;
      
      if (_bLogServerAvailable) {
#ifdef ARDUINO
         WiFiClient client;
         if (client.connect(_strLogServer.c_str(), _nLogPort)) {
            if (client.connected()) {
               client.print(sz);
            }
            client.stop();
         } else {
            _bLogServerAvailable = false;
         }
#endif
      } else {
         if (_timer60sLogServer.isDue()) {
            _bLogServerAvailable = console.isHostAvailable(_strLogServer.c_str(), _nLogPort);
         }
      }
      
      if (bAvailable != _bLogServerAvailable) {
         if (!_bLogServerAvailable) {
            console.warn(F("log server %s OFFLINE, next attemp after 60s."), _strLogServer.c_str());
         } else {
            console.info(F("log server %s online"), _strLogServer.c_str());
         }
      }
   }
   
   void _debug(const char *buf) {
      if (console.getUsrLogLevel() >= LOGLEVEL_DEBUG) {
         print(F(ESC_ATTR_DIM));
         println(buf);
         print(F(ESC_ATTR_RESET));
      }
      if (console.getLogLevel() >= LOGLEVEL_DEBUG) _print2logServer(buf);
   }
   
   void _debug_ext(uint32_t flag, const char *buf) {
      if (console.getUsrLogLevel() >= LOGLEVEL_DEBUG_EXT) {
         print(F(ESC_ATTR_DIM));
         println(buf);
         print(F(ESC_ATTR_RESET));
      }
      if (console.getLogLevel() >= LOGLEVEL_DEBUG_EXT) _print2logServer(buf);
   }
   
   void _info(const char *buf) {
      if (console.getUsrLogLevel() >= LOGLEVEL_INFO) {
         println(buf);
         print(F(ESC_ATTR_RESET));
      }
      if (console.getLogLevel() >= LOGLEVEL_INFO) _print2logServer(buf);
   }
   
   void _warn(const char *buf) {
      if (console.getUsrLogLevel() >= LOGLEVEL_WARN) {
         print(F(ESC_TEXT_YELLOW));
         println(buf);
         print(F(ESC_ATTR_RESET));
      }
      if (console.getLogLevel() >= LOGLEVEL_WARN) _print2logServer(buf);
   }
   
   void _error(const char *buf) {
      if (console.getUsrLogLevel() >= LOGLEVEL_ERROR) {
         print(F(ESC_ATTR_BOLD));
         print(F(ESC_TEXT_BRIGHT_RED));
         println(buf);
         print(F(ESC_ATTR_RESET));
      }
      if (console.getLogLevel() >= LOGLEVEL_ERROR) _print2logServer(buf);
   }

};

#endif /* CxCapabilityFS */

