//
//  CxESPConsoleLog.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleLog.hpp"

#ifdef ESP_CONSOLE_NOFS
#error "ESP_CONSOLE_NOFS was defined. CxESPConsoleLog requires a FS to work!"
#endif

void CxESPConsoleLog::begin() {
   // set the name for this console
   setConsoleName("Ext+FS+Log");
   info(F("==== LOG  ===="));

#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient && !isConnected()) startWiFi();
#endif

   // load specific environments for this class
   mount();
   __processCommand("load logserver");
   __processCommand("load logport");
   if (!__bIsWiFiClient) {
      __processCommand("load log");
   }
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
      printf(F(ESC_ATTR_BOLD "Log level:       " ESC_ATTR_RESET "%d"), __nLogLevel);printf(F(ESC_ATTR_BOLD " Usr: " ESC_ATTR_RESET "%d\n"), __nUsrLogLevel);
      printf(F(ESC_ATTR_BOLD "Ext. debug flag: " ESC_ATTR_RESET "0x%X\n"), __nExtDebugFlag);
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
         __nLogLevel = TKTOINT(tkCmd, 2, __nLogLevel);
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
      String strValue;
      if (strEnv == ".log") {
         strValue = __nLogLevel;
         saveEnv(strEnv, strValue);
      } else if (strEnv == ".logserver") {
         saveEnv(strEnv, _strLogServer);
      } else if (strEnv == ".logport") {
         strValue = _nLogPort;
         saveEnv(strEnv, strValue);
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else if (cmd == "load") {
      String strEnv = ".";
      strEnv += TKTOCHAR(tkCmd, 1);
      String strValue;
      if (strEnv == ".log") {
         if (loadEnv(strEnv, strValue)) {
            __nLogLevel = (uint32_t)strValue.toInt();
            info(F("Log level set to %d"), __nLogLevel);
         } else {
            warn(F("Log level env variable (log) not found!"));
         }
      } else if (strEnv == ".logserver") {
         if (loadEnv(strEnv, strValue)) {
            _strLogServer = strValue;
            info(F("Log server set to %s"), _strLogServer.c_str());
            _bLogServerAvailable = (_strLogServer.length() > 0 && _nLogPort > 0); // optimistic
            _timer60sLogServer.makeDue(); // force an immidiate check at next log message
         } else {
            warn(F("Log server env variable (logserver) not found!"));
         }
      } else if (strEnv == ".logport") {
         if (loadEnv(strEnv, strValue)) {
            _nLogPort = (uint32_t)strValue.toInt();
            info(F("Log port set to %d"), _nLogPort);
            _bLogServerAvailable = (_strLogServer.length() > 0 && _nLogPort > 0); // optimistic
            _timer60sLogServer.makeDue(); // force an immidiate check at next log message
         } else {
            warn(F("Log port env varialbe (logport) not found!"));
         }
      } else {
         // command not handled here, proceed into the base class
         return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
      }
   } else {
      // command not handled here, proceed into the base class
      return CxESPConsoleFS::__processCommand(szCmd, bQuiet);
   }
   return true;
}

void CxESPConsoleLog::__debug(const char *buf) {
   if (__nUsrLogLevel >= LOGLEVEL_DEBUG) {
      print(F(ESC_ATTR_DIM));
      println(buf);
      print(F(ESC_ATTR_RESET));
   }
   if (__nLogLevel >= LOGLEVEL_DEBUG) _print2logServer(buf);
}

void CxESPConsoleLog::__debug_ext(uint32_t flag, const char *buf) {
   if (__nUsrLogLevel >= LOGLEVEL_DEBUG_EXT) {
      print(F(ESC_ATTR_DIM));
      println(buf);
      print(F(ESC_ATTR_RESET));
   }
   if (__nLogLevel >= LOGLEVEL_DEBUG_EXT) _print2logServer(buf);
}

void CxESPConsoleLog::__info(const char *buf) {
   if (__nUsrLogLevel >= LOGLEVEL_INFO) {
      println(buf);
      print(F(ESC_ATTR_RESET));
   }
   if (__nLogLevel >= LOGLEVEL_INFO) _print2logServer(buf);
}

void CxESPConsoleLog::__warn(const char *buf) {
   if (__nUsrLogLevel >= LOGLEVEL_WARN) {
      print(F(ESC_TEXT_YELLOW));
      println(buf);
      print(F(ESC_ATTR_RESET));
   }
   if (__nLogLevel >= LOGLEVEL_WARN) _print2logServer(buf);
}

void CxESPConsoleLog::__error(const char *buf) {
   if (__nUsrLogLevel >= LOGLEVEL_ERROR) {
      print(F(ESC_ATTR_BOLD));
      print(F(ESC_TEXT_BRIGHT_RED));
      println(buf);
      print(F(ESC_ATTR_RESET));
  }
   if (__nLogLevel >= LOGLEVEL_ERROR) _print2logServer(buf);
}

void CxESPConsoleLog::_print2logServer(const char *sz) {
   if (__bIsWiFiClient) return; // only serial console will log to server
   
#ifdef ARDUINO
   bool bAvailable = _bLogServerAvailable;
   
   if (_bLogServerAvailable) {
      WiFiClient client;
      if (client.connect(_strLogServer.c_str(), _nLogPort)) {
         if (client.connected()) {
            client.print(sz);
         }
         client.stop();
      } else {
         _bLogServerAvailable = false;
      }
   } else {
      if (_timer60sLogServer.isDue()) {
         _bLogServerAvailable = isHostAvailable(_strLogServer.c_str(), _nLogPort);
      }
   }
   
   if (bAvailable != _bLogServerAvailable) {
      if (!_bLogServerAvailable) {
         warn(F("log server %s OFFLINE, next attemp after 60s."), _strLogServer.c_str());
      } else {
         info(F("log server %s online"), _strLogServer.c_str());
      }
   }

#endif
}
