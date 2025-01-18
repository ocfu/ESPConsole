//
//  CxESPConsole.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsole.hpp"

CxESPHeapTracker g_Heap; // init as early as possible...


uint8_t CxESPConsole::_nUsers = 0;
CxESPConsole* CxESPConsole::_pESPConsoleInstance = nullptr;


///
/// begin() shall be overridden by the last derived class. The begin()s of the
/// inherited classes shall be called first.
///
void CxESPConsole::begin() {
   println();println();
   info(F("==== BASE ===="));

#ifdef ARDUINO
#ifndef ESP_CONSOLE_NOWIFI
   if (_pWiFiServer) {
#ifdef ESP32
      _strHostName = WiFi.getHostname();
#else
      _strHostName = WiFi.hostname();
#endif
#endif
   }
   if (_nUsers == _nMaxUsers) {
      warn(F("Connection will be closed, max. number of clients reached."));
      _abortClient();
   }
#endif
   
   // silence the log messages on the console by default
   __nUsrLogLevel = 0;
   
   _nUsers++;
   setConsoleName(""); // shall be set by the last derived class
   cls();
   wlcm();
   println();
   println();
   println(F("Enter " ESC_ATTR_BOLD "?" ESC_ATTR_RESET " to get help. Have a nice day :-)"));
   __prompt();
}

void CxESPConsole::wlcm() {
   // show the wellcome message
#ifndef ESP_CONSOLE_NOWIFI
   printf(F("ESP console %s - " ESC_ATTR_BOLD "%s %s" ESC_ATTR_RESET), __szConsoleName, getAppName(), getAppVer());print(" - ");printDateTime(*__ioStream);println();
#else
   printf(F("ESP console %s + WiFi - " ESC_ATTR_BOLD "%s %s" ESC_ATTR_RESET), __szConsoleName, getAppName(), getAppVer());print(" - ");printDateTime();println();
#endif
   println();
   printInfo();
}

void CxESPConsole::printInfo() {
   print(F(ESC_ATTR_BOLD "  Hostname: " ESC_ATTR_RESET));printHostName();printf(F(ESC_ATTR_BOLD " IP: " ESC_ATTR_RESET));printIp();printf(F(ESC_ATTR_BOLD " SSID: " ESC_ATTR_RESET));printSSID();println();
   print(F(ESC_ATTR_BOLD "    Uptime: " ESC_ATTR_RESET));printUpTimeISO(*__ioStream);printf(F(" - %d user(s)"), _nUsers);    printf(F(ESC_ATTR_BOLD " Last Restart: " ESC_ATTR_RESET));printStartTime(*__ioStream);println();
   printHeap();println();
}

void CxESPConsole::printf(const char *fmt...) {
   char buffer[128]; // Temporärer Puffer für die formatierte Ausgabe
   va_list args;
   va_start(args, fmt);
   vsnprintf(buffer, sizeof(buffer), fmt, args); // Formatierte Zeichenkette erzeugen
   va_end(args);
   __ioStream->print(buffer); // Formatierte Zeichenkette ausgeben
}

void CxESPConsole::printf(const FLASHSTRINGHELPER * fmt...) {
   char buffer[128]; // Temporärer Puffer für die formatierte Ausgabe
   va_list args;
   va_start(args, fmt);
   vsnprintf(buffer, sizeof(buffer), (PGM_P)fmt, args); // Formatierte Zeichenkette erzeugen
   va_end(args);
   __ioStream->print(buffer); // Formatierte Zeichenkette ausgeben
};

void CxESPConsole::printUptimeExt() {
   // should be "hh:mm:ss up <days> days, hh:mm,  <users> user,  load average: <load>"
   uint32_t seconds = uint32_t (millis() / 1000);
   uint32_t days = seconds / 86400;
   seconds %= 86400;
   uint32_t hours = seconds / 3600;
   seconds %= 3600;
   uint32_t minutes = seconds / 60;
   seconds %= 60;
   printTime(*__ioStream);
   printf(F(" up %d days, %02d:%02d,"), days, hours, minutes);
   printf(F(" %d user, load: %.2f average: %.2f, loop time: %d"), users(), load(), avgload(), avglooptime());
}

bool CxESPConsole::hasFS() {
   return false;
}

void CxESPConsole::printHeap() {
   print(F(ESC_ATTR_BOLD " Heap Size: " ESC_ATTR_RESET));printHeapSize();print(F(" bytes"));
   print(F(ESC_ATTR_BOLD " Used: " ESC_ATTR_RESET));printHeapUsed();print(F(" bytes"));
   print(F(ESC_ATTR_BOLD " Free: " ESC_ATTR_RESET));printHeapAvailable();print(F(" bytes"));
}

void CxESPConsole::printHeapAvailable(bool fmt) {
   if (g_Heap.available() < 10000) print(F(ESC_TEXT_BRIGHT_YELLOW));
   if (g_Heap.available() < 3000) print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
   if (fmt) {
      printf(F("%7lu"), g_Heap.available());
   } else {
      printf(F("%lu"), g_Heap.available());
   }
   print(F(ESC_ATTR_RESET));
}

void CxESPConsole::printHeapSize(bool fmt) {
   if (fmt) {
      printf(F("%s%7lu%s"), g_Heap.size());
   } else {
      printf(F("%lu"), g_Heap.size());
   }
}

void CxESPConsole::printHeapUsed(bool fmt) {
   if (fmt) {
      printf(F("%7lu"), g_Heap.used());
   } else {
      printf(F("%lu"), g_Heap.used());
   }
}

bool CxESPConsole::__processCommand(const char *szCmd, bool bQuiet) {
   // validate the call
   if (!szCmd) return false;
   
   // get the command and arguments into the token buffer
   CxStrToken tkCmd(szCmd, " ");
   
   // validate again
   if (!tkCmd.count()) return false;
   
   // we have a command, find the action to take
   String cmd = TKTOCHAR(tkCmd, 0);
   
   // removes heading and trailing white spaces
   cmd.trim();
   
   if (cmd == "reboot") {
      String opt = TKTOCHAR(tkCmd, 1);
      
      // force reboot
      if (opt == "-f") {
         reboot();
      } else {
         // TODO: prompt user to be improved
         __promptUserYN("Are you sure you want to reboot?", [](bool confirmed) {
            if (confirmed) {
               if (CxESPConsole::getInstance()) CxESPConsole::getInstance()->reboot();
            }
         });
      }
   } else if (cmd == "cls") {
      cls();
   } else if (cmd == "info") {
      printInfo();
      println();
   } else if (cmd == "uptime") {
      printUptimeExt();
      println();
   } else if (cmd == "time") {
      printTime(*__ioStream);
      println();
   } else if (cmd == "date") {
      printDate(*__ioStream);
      println();
   } else if (cmd == "heap") {
      printHeap();
      println();
   } else if (cmd == "hostname") {
#ifndef ESP_CONSOLE_NOWIFI
      printHostName();
      println();
#endif
   } else if (cmd == "ip") {
#ifndef ESP_CONSOLE_NOWIFI
      printIp();
      println();
#endif
   } else if (cmd == "ssid") {
#ifndef ESP_CONSOLE_NOWIFI
      printSSID();
      println();
#endif
   } else if (cmd == "exit") {
#ifndef ESP_CONSOLE_NOWIFI
      info(F("exit wifi client"));
      _abortClient();
#else
      printf(F("exit has no function!"));
#endif
   } else if (cmd == "users") {
      printf(F("%d users (max: %d)\n"), _nUsers, _nMaxUsers);
   }  else if (cmd == "?" || cmd == USR_CMD_HELP) {
      println(F("General commands:" ESC_TEXT_BRIGHT_WHITE " ?, reboot, cls, info, uptime, time, exit, date, users, heap, hostname, ip, ssid " ESC_ATTR_RESET));
   } else if (cmd == "usr") {
      // set user specific commands here. The first parameter is the command number, the second the flag
      // and the optional third how to set/clear. (0: clear flag, 1: set flag, default (-1): set the flag as value.)
      // usr <cmd> [<flag/value> [<0|1>]]
      int32_t nCmd = TKTOINT(tkCmd, 1, -1);
      uint32_t nValue = TKTOINT(tkCmd, 2, 0);
      int8_t set = TKTOINT(tkCmd, 3, -1);
      
      switch (nCmd) {
            // usr 0: be quite, switch all loggings off on the console. log to server/file remains
         case 0:
            __nUsrLogLevel = LOGLEVEL_OFF;
            break;
            
            // usr 1: set the usr log level to show logs on the console
         case 1:
            if (nValue) {
               __nUsrLogLevel = (nValue>LOGLEVEL_MAX) ? LOGLEVEL_MAX : nValue;
            } else {
               printf(F("usr log level: %d\n"), __nUsrLogLevel);
            }
            break;
            
            // usr 2: set extended debug flag
         case 2:
            if (set < 0) {
               __nExtDebugFlag = nValue;
            } else if (set == 0) {
               __nExtDebugFlag &= ~nValue;
            } else {
               __nExtDebugFlag |= nValue;
            }
            if (__nExtDebugFlag) {
               __nLogLevel = LOGLEVEL_DEBUG_EXT;
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

void CxESPConsole::__handleConsoleInputs() {
   
   while (__ioStream->available() > 0) {
      char c = __ioStream->read();
      
      // Wenn eine Abfrage aktiv ist, Eingabe darauf anwenden
      if (_bWaitingForUsrResponseYN) {
         _handleUserResponse(c);
         continue;
      }
      
      if (c == '\n') { // Kommando abgeschlossen
         _szCmdBuffer[_iCmdBufferIndex] = '\0'; // Null-Terminierung
         __ioStream->println();
         if (!__processCommand(_szCmdBuffer)) {
            if (_szCmdBuffer[0]) {
               println(F("command was not valid!"));
            }
         }
         _storeCmd(_szCmdBuffer);
         _clearCmdBuffer();
         __prompt();
         _iCmdHistoryIndex = -1; // Reset der Historiennavigation
      } else if (c == '\b' || c == 127) { // Backspace or del
         if (_iCmdBufferIndex > 0) {
            _iCmdBufferIndex--;
            _szCmdBuffer[_iCmdBufferIndex] = '\0';
            _redrawCmd();
         }
      } else if (c == 27) { // Escape-Sequenz erkannt
         _nStateEscSequence = 1;
      } else if (_nStateEscSequence && c == '[') { // ESC für ANSI-Sequenz
         _nStateEscSequence = 2;
      } else if (_nStateEscSequence == 2 && c == 'A') { // cursor up
         _navigateCmdHistory(1);
         _nStateEscSequence = 0;
      } else if (_nStateEscSequence == 2 && c == 'B') { // Cursor down
         _navigateCmdHistory(-1);
         _nStateEscSequence = 0;
      } else if (_nStateEscSequence == 2 && c == 'C') { // Cursor right
         _nStateEscSequence = 0;
      } else if (_nStateEscSequence == 2 && c == 'D') { // Cursor left
         _nStateEscSequence = 0;
      }  else if (_nStateEscSequence == 2) { // Cursor down
         _nStateEscSequence = 0;
      } else if (_iCmdBufferIndex < _nMAXLENGTH - 1) { // Zeichen hinzufügen
         _szCmdBuffer[_iCmdBufferIndex++] = c;
         _szCmdBuffer[_iCmdBufferIndex] = '\0'; // Null-Terminierung
         __ioStream->print(c); // Zeichen anzeigen
      }
   }
}

void CxESPConsole::_measureCPULoad() {
   // TODO: improve CPU load measurement
   
   uint32_t now = (uint32_t) micros();
   
   // Zeitdifferenz zur aktiven Zeit hinzufügen
   _nActiveTime += micros() - _nStartActive;
   
   // Wenn der Beobachtungszeitraum erreicht ist
   if (now - _nLastMeasurement >= _nTotalTime) {
      if (_nLoops > 0) _navgLoopTime = (int32_t) (_nActiveTime / _nLoops);
      
      // Update der Gesamtzeit und aktiven Zeit
      _nTotalActiveTime += _nActiveTime;
      _nTotalObservationTime += _nTotalTime;
      
      // Durchschnittliche Prozesslast berechnen
      _fAvgLoad = (_nTotalActiveTime / (float)_nTotalObservationTime);
      
      // Ausgabe der aktuellen und durchschnittlichen Prozesslast
      _fLoad = (_nActiveTime / (float)_nTotalTime);
      
      // Werte zurücksetzen für den nächsten Messzyklus
      _nActiveTime = 0;
      _nLastMeasurement = now;
      _nLoops = 0;
   } else {
      _nLoops++;
   }
   
   // Zeit für die nächste Aufgabe messen
   _nStartActive = (uint32_t) micros();
      

   // Keine Pausen oder Blockierungen

}

#ifndef ESP_CONSOLE_NOWIFI
void CxESPConsole::_abortClient() {
   if (!__isWiFiClient()) {
      println("No exit on a serial connection.");
      return;
   }
#ifdef ARDUINO
   WiFiClient* client = reinterpret_cast<WiFiClient*>(__ioStream);
   
   if (client && client->connected()) {
      client->abort(); // Verbindung für WiFiClient beenden
   }
#endif
}
#endif

void CxESPConsole::loop() {
#ifdef ARDUINO
#ifndef ESP_CONSOLE_NOWIFI
   // check, if a new wifi client is or the current one is (still) connected
   if (_pWiFiServer) {
      char commandBuffer[128] = {0};
      bool commandReceived = false;
      int index = 0;

      WiFiClient client = _pWiFiServer->available();
      
      // first check, if remote connection has a command
      if (client) {
         info(F("New client connected."));
         memset(commandBuffer, 0, sizeof(commandBuffer));
         int index = 0;
         CxTimer timerTO(1000); // set timeout

         while (client.connected()) {
            if (timerTO.isDue()) {
               info(F("timeout waiting for commands"));
               break; // no command received. get into interactive console session.
            }

            while (client.available() > 0) {
               char c = client.read();
               if (c == '\n' || c == '\r' || index >= sizeof(commandBuffer) - 1) {
                  commandBuffer[index] = '\0';
                  commandReceived = true;
                  break;
               }
               commandBuffer[index++] = c;
            }
            
            if (commandReceived) {
               info(F("remote command received: %s"), commandBuffer);
               // redirect stream to client
               Stream* pStream = __ioStream;
               __ioStream = &client;
               if ( !__processCommand(commandBuffer)) {
                  println(F("command was not valid!"));
               };
               // restore stream
               __ioStream = pStream;
               client.stop();
               
               info(F("Client disconnected after command."));
               break;
            }
         }
      }
      
      // start an interactive console
      if (!commandReceived && (!_activeClient || !_activeClient.connected())) {
         if (client) {
            info(F("Start interactive console"));
            _activeClient = client; // Aktiven Client aktualisieren
            delete __espConsoleWiFiClient; // Alte Instanz löschen
            __espConsoleWiFiClient = _createInstance(_activeClient, getAppName(), getAppVer()); // Neue Instanz mit WiFiClient
            if (__espConsoleWiFiClient) {
               __espConsoleWiFiClient->setHostName(getHostName());
               __espConsoleWiFiClient->begin();
            } else {
               error(F("*** error: _createInstance() for new wifi client failed!"));
            }
         } else if (__espConsoleWiFiClient) {
            info(F("Client disconnected."));
            delete __espConsoleWiFiClient;
            __espConsoleWiFiClient = nullptr;
         }
      }
      
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->loop(); // Befehle in der Hauptschleife abarbeiten
   }
#endif
#endif
   __handleConsoleInputs();
   _measureCPULoad();
}

#ifndef ESP_CONSOLE_NOWIFI
void CxESPConsole::printHostName() {
   print(getHostName());
}

void CxESPConsole::printIp() {
#ifdef ARDUINO
   print(WiFi.localIP().toString().c_str());
#endif
}

void CxESPConsole::printSSID() {
#ifdef ARDUINO
   if (isConnected()) {
      printf(F("%s (%d dBm)"), WiFi.SSID().c_str(), WiFi.RSSI());
   }
#endif
}

void CxESPConsole::printMode() {
#ifdef ARDUINO
   switch(WiFi.getMode()) {
      case WIFI_OFF:
         print(F("OFF"));;
         break;
      case WIFI_STA:
         print(F("Station (STA)"));
         break;
      case WIFI_AP:
         print(F("Access Point (AP)"));
         break;
      case WIFI_AP_STA:
         print(F("AP+STA"));
         break;
      default:
         print(F("unknown"));
         break;
   }
#endif
}

bool CxESPConsole::isHostAvailable(const char* szHost, int nPort) {
#ifdef ARDUINO
   if (WiFi.status() == WL_CONNECTED && nPort > 0 && szHost && szHost[0] != '\0') { //Check WiFi connection status
      WiFiClient client;
      if (client.connect(szHost, nPort)) {
         client.stop();
         return true;
      }
   }
#endif
   return false;
}

// logging functions
uint32_t CxESPConsole::_addPrefix(char c, char* buf, uint32_t lenmax) {
   uint32_t len = getTime(buf, lenmax, true);
   snprintf(buf+len, lenmax, "[%c] ", c);
   return (uint32_t)strlen(buf);
}

void CxESPConsole::debug(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('D', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   __debug(buf);
   
   va_end(args);
}

void CxESPConsole::debug(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('D', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __debug(buf);
   
   va_end(args);
}

void CxESPConsole::debug_ext(uint32_t flag, const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug_ext(flag, fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('X', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   __debug_ext(flag, buf);
      
   va_end(args);
   
}

void CxESPConsole::debug_ext(uint32_t flag, const FLASHSTRINGHELPER *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug_ext(flag, fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('X', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __debug_ext(flag, buf);
   
   va_end(args);
   
}

void CxESPConsole::info(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->info(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('I', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   __info(buf);
   
   va_end(args);
}

void CxESPConsole::info(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->info(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('I', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __info(buf);
   
   va_end(args);
}

void CxESPConsole::warn(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->warn(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('W', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);

   __warn(buf);
   
   va_end(args);
}

void CxESPConsole::warn(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->warn(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('W', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __warn(buf);
   
   va_end(args);
}

void CxESPConsole::error(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->error(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('E', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);

   __error(buf);
   
   va_end(args);
}

void CxESPConsole::error(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!__isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->error(fmt, args); // forward to wifi client console
   }
   
   char buf[256];
   
   uint32_t len = _addPrefix('E', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);

   __error(buf);
   
   va_end(args);
}

void CxESPConsole::reboot() {
   warn(F("reboot..."));
#ifdef ARDUINO
   delay(1000); // let some time to handle last network messages
#ifndef ESP_CONSOLE_NOWIFI
   WiFi.disconnect();
#endif
   ESP.restart();
#endif
}

#endif /* ESP_COSNOLE_NOWIFI */
