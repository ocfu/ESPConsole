//
//  CxCapability.cpp
//  ESP
//
//  Created by ocfu on 09.01.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

size_t CxCapability::write(uint8_t c) {
   return console.write(c);
}

size_t CxCapability::write(const uint8_t *buffer, size_t size) {
   return console.write(buffer, size);
}

bool CxCapability::processCmd(const char* szCmdLine) {
   char buffer[64];  // Copy the command for strtok
   strncpy(buffer, szCmdLine, sizeof(buffer) - 1);
   buffer[sizeof(buffer) - 1] = '\0';
   
   char* command = strtok(buffer, " ");  // Get the main command
   char* args = strtok(nullptr, "");     // Get the remaining arguments
   
   if (!command) return false;  // No command found
   
   for (const auto& cmdInSet : commands) {
      if (strcmp(command, cmdInSet) == 0) {
         if (execute(command, args ? args : "")) return true;
      }
   }
   return false;
}


void CxCapabilityBasic::setup(){__bLocked = true;}
void CxCapabilityBasic::loop(){}

bool CxCapabilityBasic::execute(const char *szCmd, const char *args) {
   
   bool bQuiet = false;
   
   // validate the call
   if (!szCmd) return false;
   
   // get the arguments into the token buffer
   CxStrToken tkArgs(args, " ");
   
   // we have a command, find the action to take
   String cmd = szCmd;
   
   // removes heading and trailing white spaces
   cmd.trim();
   
   if (cmd == "cap") {
      if (tkArgs.count() > 0) {
         String strSubCmd = TKTOCHAR(tkArgs, 0);
         if (strSubCmd == "load" && tkArgs.count() > 1) {
            console.createCapInstance(TKTOCHAR(tkArgs, 1), "");
         } else if (strSubCmd == "unload" && tkArgs.count() > 1) {
            console.deleteCapInstance(TKTOCHAR(tkArgs, 1));
         } else if (strSubCmd == "list") {
            console.listCap();
         }
      } else {
         console.println(F("usage: cap <cmd> [<param> <...>]"));
         console.println(F("commands:"));
         console.println(F(" load <cap. name>"));
         console.println(F(" unload <cap. name>"));
         console.println(F(" list"));
      }
      return true;
   } else if (cmd == "reboot") {
      String opt = TKTOCHAR(tkArgs, 0);
      
      // force reboot
      if (opt == "-f" || bQuiet) {
         console.reboot();
      } else {
         // TODO: prompt user to be improved
         //         console.__promptUserYN("Are you sure you want to reboot?", [this](bool confirmed) {
         //            if (confirmed) {
         //               console.reboot();
         //            }
         //         });
      }
   } else if (cmd == "cls") {
      console.cls();
   } else if (cmd == "info") {
      console.printInfo();
      println();
   } else if (cmd == "uptime") {
      console.printUptimeExt();
      println();
   } else if (cmd == "time") {
      if(console.getStream()) console.printTime(*console.getStream());
      println();
   } else if (cmd == "date") {
      if(console.getStream()) console.printDate(*console.getStream());
      println();
   } else if (cmd == "heap") {
      console.printHeap();
      println();
   } else if (cmd == "hostname") {
#ifndef ESP_CONSOLE_NOWIFI
      console.printHostName();
      println();
#endif
   } else if (cmd == "ip") {
#ifndef ESP_CONSOLE_NOWIFI
      console.printIp();
      console.println();
#endif
   } else if (cmd == "ssid") {
#ifndef ESP_CONSOLE_NOWIFI
      console.printSSID();
      println();
#endif
   } else if (cmd == "exit") {
#ifndef ESP_CONSOLE_NOWIFI
      console.info(F("exit wifi client"));
      //console._abortClient();
#else
      printf(F("exit has no function!"));
#endif
   } else if (cmd == "users") {
      //printf(F("%d users (max: %d)\n"), _nUsers, _nMaxUsers);
   } else if (cmd == "usr") {
      // set user specific commands here. The first parameter is the command number, the second the flag
      // and the optional third how to set/clear. (0: clear flag, 1: set flag, default (-1): set the flag as value.)
      // usr <cmd> [<flag/value> [<0|1>]]
      int32_t nCmd = TKTOINT(tkArgs, 1, -1);
      uint32_t nValue = TKTOINT(tkArgs, 2, 0);
      int8_t set = TKTOINT(tkArgs, 3, -1);
      
      switch (nCmd) {
            // usr 0: be quite, switch all loggings off on the console. log to server/file remains
         case 0:
            console.setUsrLogLevel(LOGLEVEL_OFF);
            break;
            
            // usr 1: set the usr log level to show logs on the console
         case 1:
            if (nValue) {
               console.setUsrLogLevel(nValue>LOGLEVEL_MAX ? LOGLEVEL_MAX : nValue);
            } else {
               printf(F("usr log level: %d\n"), console.getUsrLogLevel());
            }
            break;
            
            // usr 2: set extended debug flag
         case 2:
            if (set < 0) {
               console.setDebugFlag(nValue);
            } else if (set == 0) {
               console.resetDebugFlag(nValue);
            } else {
               console.setDebugFlag(console.getDebugFlag() | nValue);
            }
            if (console.getDebugFlag()) {
               console.setLogLevel(LOGLEVEL_DEBUG_EXT);
            }
            break;
            
         default:
            println(F("usage: usr <cmd> [<flag/value> [<0|1>]]"));
            println(F(" 0           be quite, switch all log messages off on the console."));
            println(F(" 1  <1..5>   set the log level to show log messages on the console."));
            println(F(" 2  <flag>   set the extended debug flag(s) to the value."));
            println(F(" 2  <flag> 0 clear an extended debug flag."));
            println(F(" 2  <flag> 1 add an extended debug flag."));
            break;
      }
   } else {
      return false;
   }
   return true;
}

