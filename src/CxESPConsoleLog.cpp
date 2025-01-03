//
//  CxESPConsoleLog.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleLog.hpp"

#ifndef ESP_CONSOLE_NOFS
void CxESPConsoleLog::begin() {
   // set the name for this console
   setConsoleName("Ext+FS+Log");
   
   // load specific environments for this class
   mount();
   __processCommand("load log", true);
   
   // call the begin() from base class(es)
   CxESPConsoleFS::begin();

}

void CxESPConsoleLog::printInfo() {
   CxESPConsoleFS::printInfo();
   
   // specific for this console
}

bool CxESPConsoleLog::__processCommand(const char *szCmd, bool bQuiet) {
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
      CxESPConsoleFS::__processCommand(szCmd);
      println(F("Log commands:" ESC_TEXT_BRIGHT_WHITE "     log, usr" ESC_ATTR_RESET));
   } else if (cmd == "log") {
      printf(F("Log level:           %d\n"), _nLogLevel);
      printf(F("Extended debug flag: 0x%X\n"), _nExtDebugFlag);
      println(F("example log message:"));
      _LOG_DEBUG(F("Test Log message"));
   } else if (cmd == "set") {
      ///
      /// known env variables:
      /// - log <loglevel>
      ///
      
      String strVar = TKTOCHAR(tkCmd, 1);
      if (strVar == "log") {
         _nLogLevel = TKTOINT(tkCmd, 2, _nLogLevel);
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else if (cmd == "save") {
      String strEnv = ".";
      strEnv += TKTOCHAR(tkCmd, 1);
      if (strEnv == ".log") {
         saveEnv(strEnv.c_str(), String(_nLogLevel).c_str());
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else if (cmd == "load") {
      String strEnv = ".";
      strEnv += TKTOCHAR(tkCmd, 1);
      String strValue;
      if (strEnv == ".log") {
         if (loadEnv(strEnv.c_str(), strValue)) {
            _nLogLevel = (uint32_t)strValue.toInt();
            printf(F("Log level set to %d\n"), _nLogLevel);
         } else {
            println(F("Log level env variable (log) not found!"));
         }
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else if (cmd == "usr") {
      // set user specific commands here. The first parameter is the command number, the second the flag
      // and the optional third how to set/clear. (0: clear flag, 1: set flag, default (-1): set the flag as value.)
      // usr <cmd> [<flag> [<0|1>]]
      int32_t nCmd = TKTOINT(tkCmd, 1, -1);
      uint32_t flag = TKTOINT(tkCmd, 2, 0);
      int8_t set = TKTOINT(tkCmd, 3, -1); 
      
      switch (nCmd) {
         // usr 0: be quite, switch all loggings off
         case 0:
            _nLogLevel = LOGLEVEL_OFF;
            _nExtDebugFlag = 0x0;
            break;
            
         // usr 1: set extended debug flag
         case 1:
            if (set < 0) {
               _nExtDebugFlag = flag;
            } else if (set == 0) {
               _nExtDebugFlag &= ~flag;
            } else {
               _nExtDebugFlag |= flag;
            }
            if (_nExtDebugFlag) {
               _nLogLevel = LOGLEVEL_DEBUG_EXT;
            }
            break;
            
         default:
            println(F("usage: usr <cmd> [<flag> [<0|1>]]"));
            break;
      }
   }
   else {
      // command not handled here, proceed into the base class
      return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
   }
   return true;
}

void CxESPConsoleLog::_printPrefix(char c) {
   printTime(false);
   printf("[%c] ", c);
}

void CxESPConsoleLog::debug(const char *fmt, ...) {
   
   if (_nLogLevel < LOGLEVEL_DEBUG) return;
   
   va_list args;
   va_start(args, fmt);
   
   char buf[256];
   
   _printPrefix('D');
   
   vsnprintf(buf, sizeof(buf), fmt, args);
   println(buf);
   
   va_end(args);
}

void CxESPConsoleLog::debug(const FLASHSTRINGHELPER * fmt...) {
   
   if (_nLogLevel < LOGLEVEL_DEBUG) return;

   va_list args;
   va_start(args, fmt);
   
   char buf[256];
   
   _printPrefix('D');

   vsnprintf(buf, sizeof(buf), (PGM_P) fmt, args);
   println(buf);
   
   va_end(args);
}

#endif /*ESP_CONSOLE_NOFS*/
