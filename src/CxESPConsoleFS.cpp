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
   info(F("====  FS  ===="));

#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient && !isConnected()) startWiFi();
#endif

   // load specific environments for this class
   mount();
   _processCommand("load ntp");
   _processCommand("load tz");
   _processCommand("load led");

   __updateTime();
   
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
   printFsInfo();
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
      if (fmt) {
         printf(F("%7ld"), getDf());
      } else {
         printf(F("%ld"), getDf());
      }
   } else {
      __printNoFS();
   }
}

uint32_t CxESPConsoleFS::getDf() {
   if (hasFS()) {
      FSInfo fsinfo;
      _getFSInfo(fsinfo);
      return (uint32_t) (fsinfo.totalBytes - fsinfo.usedBytes);
   } else {
      return 0;
   }
}

void CxESPConsoleFS::ls(bool bAll, bool bLong) {
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
                  printFileDateTime(*__ioStream, file.getCreationTime(), file.getLastWrite());
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
            printFileDateTime(*__ioStream, file.getCreationTime(), file.getLastWrite());
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
            print((char)file.read());
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

void CxESPConsoleFS::printFsInfo() {
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
         error("LittleFS mount failed");
         return;
      }
#else
#endif
   } else {
      //println(F("LittleFS already mounted!"));
   }
}

void CxESPConsoleFS::umount() {
   if (hasFS()) {
#ifdef ARDUINO
      LittleFS.end();
#else
#endif
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

bool CxESPConsoleFS::_processCommand(const char *szCmd, bool bQuiet) {
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
   
   if (cmd == "du") {printDu(a);a ? println() : println(" .");
   } else if (cmd == "df") {printDf();println(F(" bytes"));
   } else if (cmd == "size") {printSize();println(F(" bytes"));
   } else if (cmd == "ls") {
      String strOpt = TKTOCHAR(tkCmd, 1);
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
   } else if (cmd == "fs") {
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
      strEnv += TKTOCHAR(tkCmd, 1);
      String strValue;
      if (strEnv == ".ntp") {
         strValue = getNtpServer();
         saveEnv(strEnv, strValue);
      } else if (strEnv == ".tz") {
         strValue = getTimeZone();
         saveEnv(strEnv, strValue);
      } else if (strEnv == ".led") {
         strValue = "Pin:";
         strValue += Led1.getPin();
         if (Led1.isInverted()) strValue += ",inverted";
         saveEnv(strEnv, strValue);
      } else {
         println(F("save environment variable. \nusage: save <env>"));
         println(F("known env variables:\n ntp \n tz "));
         println(F("example: save ntp"));
      }
   } else if (cmd == "load") {
      String strEnv = ".";
      strEnv += TKTOCHAR(tkCmd, 1);
      String strValue;
      if (strEnv == ".ntp") {
         if (loadEnv(strEnv, strValue)) {
            setNtpServer(strValue.c_str());
            info(F("NTP server set to %s"), getNtpServer());
         } else {
            warn(F("NTP server env variable (ntp) not found!"));
         }
      } else if (strEnv == ".tz") {
         if (loadEnv(strEnv, strValue)) {
            setTimeZone(strValue.c_str());
            info(F("Timezone set to %s"), getTimeZone());
         } else {
            warn(F("Timezone env variable (tz) not found!"));
         }
      } else if (strEnv == ".led") {
         if (loadEnv(strEnv, strValue)) {
            // Extract pin number
            int pinIndex = strValue.indexOf("Pin:");
            int pin = -1; // Default invalid pin
            if (pinIndex != -1) {
               int start = pinIndex + 4; // Position after "Pin:"
               int end = strValue.indexOf(',', start); // Find the comma after the pin
               if (end == -1) end = strValue.length(); // No comma, take the rest of the string
               pin = (uint8_t)strValue.substring(start, end).toInt(); // Convert the extracted substring to an integer
            }
            
            // Check if inverted
            bool inverted = strValue.indexOf("inverted") != -1;
            
            // set led pin if changed
            if (Led1.getPin() != pin) {
               info(F("set Led1 to pin %d"), pin);
               Led1.setPin(pin);
            }
            
            if (Led1.isInverted() != inverted) {
               info(F("set Led1 on pin %d to %s logic"), Led1.getPin(), inverted?"inverted":"non-inverted");
               Led1.setInverted(inverted);
            }
         }
      }
      else {
         println(F("load environment varialbe.\nusage: load <env>"));
         println(F("known env variables:\n ntp \n tz "));
         println(F("example: load ntp"));
      }
   } else if (cmd == "$UPLOAD$") {
      _handleFile();
   } else if (cmd == "$DOWNLOAD$") {
      _handleFile();
   } else {
      return false;
   }
   return true;
}

void CxESPConsoleFS::saveEnv(String& strEnv, String& strValue) {
   if (hasFS()) {
      debug(F("save env variable %s, value=%s"), strEnv.c_str(), strValue.c_str());
#ifdef ARDUINO
      File file = LittleFS.open(strEnv.c_str(), "w");
      if (file) {
         file.print(strValue.c_str());
         file.close();
      }
#endif
   } else {
      __printNoFS();
   }
}

bool CxESPConsoleFS::loadEnv(String& strEnv, String& strValue) {
   if (hasFS()) {
      debug(F("load env variable %s"), strEnv.c_str());
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
      __printNoFS();
   }
   return false;
}

bool CxESPConsoleFS::_handleFile() {
#ifdef ARDUINO
   char buffer[512];
   String header = "";
   String filename = "";
   size_t expectedSize = 0;
   size_t receivedSize = 0;
   File file;
   bool bError = false;

   WiFiClient* client = static_cast<WiFiClient*>(__ioStream);
   
   // read header
   while (client->connected() && header.indexOf("\n") == -1) {
      if (client->available()) {
         char c = client->read();
         header += c;
      }
   }
   
   _LOG_DEBUG(F("receive header: %s"), header.c_str());

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
         error(F("not enough space available for the file!"));
         return false;
      }
      
      info(F("receive file: %s (size: %d Bytes)"), filename.c_str(), expectedSize);
            
      // file open
      file = LittleFS.open(filename, "w");
      if (!file) {
         println(F("error: create file"));
         error(F("error: create file %s"), filename.c_str());
         return false;
      }
   } else {
      println(F("error: invalid header"));
      error(F("error: invalid header received during file transfer"));
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
         printProgressBar(receivedSize, expectedSize, filename.c_str());
         timerTO.restart(); // reset timeout
      } else if (timerTO.isDue()) { // timeout
         error(F("timeout receiving a file"));
         bError = true;
         break;
      }
      delay(1);
   }
   println(" done!");
   file.close();
   
   // Empfang überprüfen
   if (receivedSize == expectedSize) {
      info(F("file transfer finished."));
   } else {
      printf(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "Warning: received size of data (%d bytes) not same as expected file size (%d bytes) !\n" ESC_ATTR_RESET), receivedSize, expectedSize);
      error(F("received size of data (%d bytes) not same as expected file size (%d bytes)!"), receivedSize, expectedSize);
   }
#endif
   
   return true;
}

bool CxESPConsoleFS::_sendFile(WiFiClient* client, const char* filename) {
#ifdef ARDUINO
   
   _LOG_DEBUG(F("download file: %s"), filename);
   
   File file = LittleFS.open(filename, "r");
   if (!file) {
      client->println("ERROR: File not found");
      warn("File not found: %s", filename);
      return false;
   }
   
   size_t fileSize = file.size();
   client->printf("SIZE: %d\n", fileSize);
   info("Sending file: %s (%d bytes)\n", filename, fileSize);
   
   char buffer[512];
   size_t bytesRead;
   
   // Send file data
   while ((bytesRead = file.readBytes(buffer, sizeof(buffer))) > 0) {
      client->write((uint8_t *)buffer, bytesRead);
   }
   
   file.close();
   info("File transfer complete.");
#endif
   return true;
}

#endif /*ESP_CONSOLE_NOFS*/
