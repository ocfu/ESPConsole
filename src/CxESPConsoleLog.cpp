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
   __processCommand("load logserver", true);
   __processCommand("load logport", true);
   
   info(F("log started"));

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
      printf(F(ESC_ATTR_BOLD "Log level:       " ESC_ATTR_RESET "%d"), _nLogLevel);printf(F(ESC_ATTR_BOLD " Usr: " ESC_ATTR_RESET "%d\n"), _nUsrLogLevel);
      printf(F(ESC_ATTR_BOLD "Ext. debug flag: " ESC_ATTR_RESET "0x%X\n"), _nExtDebugFlag);
      printf(F(ESC_ATTR_BOLD "Log server:      " ESC_ATTR_RESET "%s (%s)\n"), _strLogServer.c_str(), _bLogServerAvailable?"online":"offline");
      printf(F(ESC_ATTR_BOLD "Log port:        " ESC_ATTR_RESET "%d\n"), _nLogPort);
      info(F("test log message"));
   } else if (cmd == "set") {
      ///
      /// known env variables:
      /// - log <level>
      /// - logserver <server>
      /// - logport <port>
      
      String strVar = TKTOCHAR(tkCmd, 1);
      if (strVar == "log") {
         _nLogLevel = TKTOINT(tkCmd, 2, _nLogLevel);
      } else if (strVar == "logserver") {
         _strLogServer = TKTOCHAR(tkCmd, 2);
      } else if (strVar == "logport") {
         _nLogPort = TKTOINT(tkCmd, 2, 0);
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else if (cmd == "save") {
      String strEnv = ".";
      strEnv += TKTOCHAR(tkCmd, 1);
      if (strEnv == ".log") {
         saveEnv(strEnv.c_str(), String(_nLogLevel).c_str());
      } else if (strEnv == ".logserver") {
         saveEnv(strEnv.c_str(), _strLogServer.c_str());
      } else if (strEnv == ".logport") {
         saveEnv(strEnv.c_str(), String(_nLogPort).c_str());
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
      } else if (strEnv == ".logserver") {
         if (loadEnv(strEnv.c_str(), strValue)) {
            _strLogServer = strValue;
            printf(F("Log server set to %s\n"), _strLogServer.c_str());
            _bLogServerAvailable = true; // optimistic
         } else {
            println(F("Log server env variable (logserver) not found!"));
         }
      } else if (strEnv == ".logport") {
         if (loadEnv(strEnv.c_str(), strValue)) {
            _nLogPort = (uint32_t)strValue.toInt();
            printf(F("Log port set to %d\n"), _nLogPort);
            _bLogServerAvailable = true; // optimistic
         } else {
            println(F("Log port env varialbe (logport) not found!"));
         }
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else if (cmd == "usr") {
      // set user specific commands here. The first parameter is the command number, the second the flag
      // and the optional third how to set/clear. (0: clear flag, 1: set flag, default (-1): set the flag as value.)
      // usr <cmd> [<flag/value> [<0|1>]]
      int32_t nCmd = TKTOINT(tkCmd, 1, -1);
      uint32_t flag = TKTOINT(tkCmd, 2, 0);
      int8_t set = TKTOINT(tkCmd, 3, -1); 
      
      switch (nCmd) {
         // usr 0: be quite, switch all loggings off on the console. log to server/file remains
         case 0:
            _nUsrLogLevel = LOGLEVEL_OFF;
            break;
            
         // usr 1: set the usr log level to show logs on the console
         case 1:
            _nUsrLogLevel = flag;
            break;
            
         // usr 2: set extended debug flag
         case 2:
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
            println(F("usage: usr <cmd> [<flag/value> [<0|1>]]"));
            break;
      }
   }
   else {
      // command not handled here, proceed into the base class
      return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
   }
   return true;
}

uint32_t CxESPConsoleLog::_addPrefix(char c, char* buf, uint32_t lenmax) {
   uint32_t len = getTime(buf, lenmax, true);
   snprintf(buf+len, lenmax, "[%c] ", c);
   return (uint32_t)strlen(buf);
}

void CxESPConsoleLog::debug(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->debug(fmt, args); // forward to wifi client console
   }

   char buf[256];

   uint32_t len = _addPrefix('D', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_DEBUG) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_DEBUG) _print2logServer(buf);
   
   va_end(args);
}

void CxESPConsoleLog::debug(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->debug(fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('D', buf, sizeof(buf));

   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);

   if (_nUsrLogLevel >= LOGLEVEL_DEBUG) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_DEBUG) _print2logServer(buf);
   
   va_end(args);
}

void CxESPConsoleLog::debug_ext(uint32_t flag, const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->debug_ext(flag, fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('X', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_DEBUG_EXT) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_DEBUG_EXT) _print2logServer(buf);
   
   va_end(args);

}

void CxESPConsoleLog::debug_ext(uint32_t flag, const FLASHSTRINGHELPER *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->debug_ext(flag, fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('X', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_DEBUG_EXT) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_DEBUG_EXT) _print2logServer(buf);
   
   va_end(args);

}

void CxESPConsoleLog::info(const char *fmt, ...) {
      
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->info(fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('I', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_INFO) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_INFO) _print2logServer(buf);
   
   va_end(args);
}

void CxESPConsoleLog::info(const FLASHSTRINGHELPER * fmt...) {

   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->info(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('I', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_INFO) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_INFO) _print2logServer(buf);
   


   va_end(args);
}

void CxESPConsoleLog::warn(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->warn(fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('W', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_WARN) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_WARN) _print2logServer(buf);

   va_end(args);
}

void CxESPConsoleLog::warn(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);

   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->warn(fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('W', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_WARN) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_WARN) _print2logServer(buf);

   va_end(args);
}

void CxESPConsoleLog::error(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->error(fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('E', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_ERROR) println(buf);

   if (_nLogLevel >= LOGLEVEL_ERROR) _print2logServer(buf);

   va_end(args);
}

void CxESPConsoleLog::error(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__bIsWiFiClient) {
      if (__espConsoleWiFiClient) static_cast<CxESPConsoleLog*>(__espConsoleWiFiClient)->error(fmt, args); // forward to wifi client console
   }

   char buf[256];
   
   uint32_t len = _addPrefix('E', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   if (_nUsrLogLevel >= LOGLEVEL_ERROR) println(buf);
   
   if (_nLogLevel >= LOGLEVEL_ERROR) _print2logServer(buf);
   
   va_end(args);
}

void CxESPConsoleLog::_print2logServer(const char *sz) {
#ifdef ARDUINO
   if (_bLogServerAvailable) {
      WiFiClient client;
      if (client.connect(_strLogServer.c_str(), _nLogPort)) {
         if (client.connected()) {
            client.print(sz);
         } else {
            _bLogServerAvailable = false;
         }
         client.stop();
      }
   } else {
      if ((millis() - _nLastLogServerCheck) > 60000) {
         _bLogServerAvailable = isHostAvailable(_strLogServer.c_str(), _nLogPort);
         _nLastLogServerCheck = millis();
      }
   }
#endif
}

#endif /*ESP_CONSOLE_NOFS*/
