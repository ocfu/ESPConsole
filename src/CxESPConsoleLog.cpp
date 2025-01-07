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
   
#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient && !isConnected()) startWiFi();
#endif

   // set the name for this console
   setConsoleName("Ext+FS+Log");
   
   // load specific environments for this class
   if (!__bIsWiFiClient) {
      mount();
      __processCommand("load log", true);
      __processCommand("load logserver", true);
      __processCommand("load logport", true);
      
      info(F("log started"));
   }

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
      if (strEnv == ".log") {
         saveEnv(strEnv.c_str(), String(__nLogLevel).c_str());
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
            __nLogLevel = (uint32_t)strValue.toInt();
            info(F("Log level set to %d"), __nLogLevel);
         } else {
            warn(F("Log level env variable (log) not found!"));
         }
      } else if (strEnv == ".logserver") {
         if (loadEnv(strEnv.c_str(), strValue)) {
            _strLogServer = strValue;
            info(F("Log server set to %s"), _strLogServer.c_str());
            _bLogServerAvailable = (_strLogServer.length() > 0 && _nLogPort > 0); // optimistic
            _nLastLogServerCheck = 0; // but force a check at next log message
         } else {
            warn(F("Log server env variable (logserver) not found!"));
         }
      } else if (strEnv == ".logport") {
         if (loadEnv(strEnv.c_str(), strValue)) {
            _nLogPort = (uint32_t)strValue.toInt();
            info(F("Log port set to %d"), _nLogPort);
            _bLogServerAvailable = (_strLogServer.length() > 0 && _nLogPort > 0); // optimistic
            _nLastLogServerCheck = 0; // but force a check at next log message
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
      if (_nLastLogServerCheck == 0 || (millis() - _nLastLogServerCheck) > 60000) {
         _bLogServerAvailable = isHostAvailable(_strLogServer.c_str(), _nLogPort);
         _nLastLogServerCheck = millis();
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

#endif /*ESP_CONSOLE_NOFS*/
