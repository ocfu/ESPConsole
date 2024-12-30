//
//  CxESPConsoleFS.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleFS.hpp"

#ifndef ESP_CONSOLE_NOFS
void CxESPConsoleFS::begin() {
   // set the name for this console
   setConsoleName("Ext+FS");
   
   // specifics for this console
   mount();
   
   // call the begin() from base class(es)
   CxESPConsoleExt::begin();
}

bool CxESPConsoleFS::hasFS() {
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

void CxESPConsoleFS::printInfo() {
   CxESPConsoleExt::printInfo();
   
   // print the special usage of this console extension
   printFsUsage();
}


void CxESPConsoleFS::printDu(bool fmt, const char* szFn) {
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
            __printNoSuchFileOrDir("du", szFn);
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
      __printNoFS();
   }
}

void CxESPConsoleFS::printSize(bool fmt) {
   if (hasFS()) {
      FSInfo fsinfo;
      _getFSInfo(fsinfo);
      if (fmt) {
         printf(F("%07ld"), fsinfo.totalBytes);
      } else {
         printf(F("%ld"), fsinfo.totalBytes);
      }
   } else {
      __printNoFS();
   }
}

void CxESPConsoleFS::printDf(bool fmt) {
   if (hasFS()) {
      FSInfo fsinfo;
      _getFSInfo(fsinfo);
      if (fmt) {
         printf(F("%7ld"), fsinfo.totalBytes - fsinfo.usedBytes);
      } else {
         printf(F("%ld"), fsinfo.totalBytes - fsinfo.usedBytes);
      }
   } else {
      __printNoFS();
   }
}

void CxESPConsoleFS::ls(bool bAll) {
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
               if (bAll) {
                  printf(F("%7d "), file.size());
                  printFileDateTime(file.getCreationTime(), file.getLastWrite());
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
         if (bAll) {
            printf(F("%7d "), file.size());
            printFileDateTime(file.getCreationTime(), file.getLastWrite());
         }
         printf(F(" %s\n"), file.name());
         total += file.size();
         file.close();
      }
#endif // end ESP32
      if (bAll) {
         printf(F("%7d (%d bytes free)\n"), total, totalBytes - usedBytes);
      }
#endif // end ARDUINO
   } else {
      __printNoFS();
   }
}

void CxESPConsoleFS::cat(const char* szFn) {
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
            print(file.read());
         }
         println();
      } else {
         __printNoSuchFileOrDir("cat", szFn);
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
      __printNoFS();
   }
}

void CxESPConsoleFS::rm(const char* szFn) {
   if (! szFn) {
      println(F("usage: rm <file>"));
      return;
   }
   if (hasFS()) {
#ifdef ARDUINO
      if (!LittleFS.remove(szFn)) {
         __printNoSuchFileOrDir("rm", szFn);
      }
#else
#endif
   } else {
      __printNoFS();
   }
}

void CxESPConsoleFS::touch(const char* szFn) {
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
      __printNoFS();
   }
}

void CxESPConsoleFS::cp(const char *szSrc, const char *szDst) {
   if (! szSrc || ! szDst) {
      println(F("usage: cp <src_file> <tgt_file>"));
      return;
   }
   if (hasFS()) {
#ifdef ARDUINO
      if (LittleFS.exists(szSrc)) {
         char buf[64];
         
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
         __printNoSuchFileOrDir("cp", szSrc);
      }
#else
#endif
   } else {
      __printNoFS();
   }
}

void CxESPConsoleFS::printFsUsage() {
   if (hasFS()) {
      print(F(ESC_ATTR_BOLD   "Filesystem: " ESC_ATTR_RESET "Little FS"));
      print(F(ESC_ATTR_BOLD " Size: " ESC_ATTR_RESET));printSize();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));printDu();print(F(" bytes"));
      print(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));printDf();print(F(" bytes"));
   } else {
      print(F(ESC_ATTR_BOLD "Filesystem: " ESC_ATTR_RESET "not mounted"));
   }
}

void CxESPConsoleFS::mount() {
   if (!hasFS()) {
#ifdef ARDUINO
      if (!LittleFS.begin()) {
         println("LittleFS mount failed");
         return;
      }
#else
#endif
   } else {
      println(F("LittleFS already mounted!"));
   }
}

void CxESPConsoleFS::umount() {
   if (hasFS()) {
#ifdef ARDUINO
      LittleFS.end();
#else
#endif
   } else {
      println(F("LittleFS already mounted!"));
   }
}

void CxESPConsoleFS::format() {
   if (hasFS()) {
#ifdef ARDUINO
      println(F("LittleFS still mounted! -> 'umount' first"));

   } else {
      __promptUserYN("Are you sure you want to format?", [](bool confirmed) {
         if (confirmed) {
#ifdef ARDUINO
            LittleFS.format();
#endif
         }
      });
#else
#endif
   }
}

bool CxESPConsoleFS::__processCommand(const char *szCmd) {
   // validate the call
   if (!szCmd) return false;
   
   // get the command and arguments into the token buffer
   CxStrToken tkCmd(szCmd, " ");
   
   // validate again
   if (!tkCmd.count()) return false;
   
   // switch log output format to console output settings (e.g. no time heading) for this call
   // it will be set back at the end.
   //ioStream->consoleFormat();
   
   // we have a command, find the action to take
   String cmd = TKTOCHAR(tkCmd, 0);
   
   // removes heading and trailing white spaces
   cmd.trim();
   
   // expect sz parameter, invalid is nullptr
   const char* a = TKTOCHAR(tkCmd, 1);
   const char* b = TKTOCHAR(tkCmd, 2);
   
   if (cmd == "?" || cmd == USR_CMD_HELP) {
      // show help first from base class(es)
      CxESPConsoleExt::__processCommand(szCmd);
      println(F("FS commands:" ESC_TEXT_BRIGHT_WHITE "      du, df, size, ls, cat, cp, rm, touch, mount, umount, format" ESC_ATTR_RESET));
   } else if (cmd == "du") {printDu(a);a ? println() : println(" .");
   } else if (cmd == "df") {printDf();println(F(" bytes"));
   } else if (cmd == "size") {printSize();println(F(" bytes"));
   } else if (cmd == "ls") {
      String strOpt = TKTOCHAR(tkCmd, 1);
      ls((strOpt == "-l"));
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
   } else if (cmd == "fs") {
      printFsUsage();
      println();
   } else {
      // command not handled here, proceed into the base class
      return CxESPConsoleExt::__processCommand(szCmd);
   }
   return true;
}

#endif /*ESP_CONSOLE_NOFS*/
