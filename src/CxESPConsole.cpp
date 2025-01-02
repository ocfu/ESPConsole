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


void CxESPConsole::begin() {
   println();println();

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
      println(F("Connection will be closed, max. number of clients reached."));
      _abortClient();
   }
#endif
   
   _nUsers++;
   setConsoleName("");
   // wellcome message(s) will be called from derived classes to here
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
   printf(F("ESP console %s - " ESC_ATTR_BOLD "%s %s" ESC_ATTR_RESET), __szConsoleName, getAppName(), getAppVer());print(" - ");printDateTime();println();
#else
   printf(F("ESP console %s + WiFi - " ESC_ATTR_BOLD "%s %s" ESC_ATTR_RESET), __szConsoleName, getAppName(), getAppVer());print(" - ");printDateTime();println();
#endif
   println();
   printInfo();
}

void CxESPConsole::printInfo() {
   print(F(ESC_ATTR_BOLD "  Hostname: " ESC_ATTR_RESET));printHostName();printf(F(ESC_ATTR_BOLD " IP: " ESC_ATTR_RESET));printIp();printf(F(ESC_ATTR_BOLD " SSID: " ESC_ATTR_RESET));printSSID();println();
   print(F(ESC_ATTR_BOLD "    Uptime: " ESC_ATTR_RESET));printUpTimeISO();printf(F(" - %d user(s)"), _nUsers);    printf(F(ESC_ATTR_BOLD " Last Restart: " ESC_ATTR_RESET));printStartTime();println();
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
   printTime();
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
      // TODO: prompt user to be improved
      __promptUserYN("Are you sure you want to reboot?", [](bool confirmed) {
         if (confirmed) {
#ifdef ARDUINO
            delay(1000); // let some time to handle last network messages
#ifndef ESP_CONSOLE_NOWIFI
            WiFi.disconnect();
#endif
            delay(1000);
            ESP.restart();
#endif
         }
      });
   } else if (cmd == "cls") {
      cls();
   } else if (cmd == "info") {
      printInfo();
      println();
   } else if (cmd == "uptime") {
      printUptimeExt();
      println();
   } else if (cmd == "time") {
      printTime();
      println();
   } else if (cmd == "date") {
      printDate();
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
      _abortClient();
#else
      printf(F("exit has no function!"));
#endif
   } else if (cmd == "users") {
      printf(F("%d users (max: %d)\n"), _nUsers, _nMaxUsers);
   }  else if (cmd == "?" || cmd == USR_CMD_HELP) {
      println(F("General commands:" ESC_TEXT_BRIGHT_WHITE " ?, reboot, cls, info, uptime, time, exit, date, users, heap, hostname, ip, ssid " ESC_ATTR_RESET));
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
   if (!_isWiFiClient()) {
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
      if (!activeClient || !activeClient.connected()) {
         WiFiClient client = _pWiFiServer->available();
         if (client) {
            println(F("New client connected."));
            activeClient = client; // Aktiven Client aktualisieren
            delete espConsoleWiFiClient; // Alte Instanz löschen
            espConsoleWiFiClient = _createInstance(activeClient, getAppName(), getAppVer()); // Neue Instanz mit WiFiClient
            if (espConsoleWiFiClient) {
               espConsoleWiFiClient->setHostName(getHostName());
               espConsoleWiFiClient->begin();
            } else {
               println(F("*** error: _createInstance() for new wifi client failed!"));
            }
         } else if (espConsoleWiFiClient) {
            println(F("Client disconnected."));
            delete espConsoleWiFiClient;
            espConsoleWiFiClient = nullptr;
         }
      }
      
      if (espConsoleWiFiClient) espConsoleWiFiClient->loop(); // Befehle in der Hauptschleife abarbeiten
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
   printf(F("%s (%d dBm)"), WiFi.SSID().c_str(), WiFi.RSSI());
#endif
}

#endif
