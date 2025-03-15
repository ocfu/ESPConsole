//
//  CxESPConsole.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsole.hpp"
#include "../capabilities/CxCapabilityBasic.hpp"

CxESPHeapTracker g_Heap(51000); // init as early as possible...
CxESPStackTracker g_Stack;

uint8_t CxESPConsole::__nUsers = 0;

CxESPConsoleMaster& ESPConsole = CxESPConsoleMaster::getInstance();

bool CxESPConsoleMaster::processCmd(const char* cmd, bool bQuiet) {
   if (!cmd) return false;
   for (auto& entry : _mapCapInstances) {
      bool bResult = false;
      
      entry.second->setIoStream(*__ioStream);
      
      bResult = entry.second->processCmd(cmd);
      if (bResult && *cmd != '?') return true;
   }
   
   if (!bQuiet && strlen(cmd) > 0 && *cmd != '?') {
      println("Unknown command: ");
      println(cmd);
   }
   return false;
}

void CxESPConsoleMaster::begin() {
   info(F("==== MASTER ===="));
   
#ifdef ARDUINO
#ifndef ESP_CONSOLE_NOWIFI
   if (_pWiFiServer) {
#ifdef ESP32
      setHostName(WiFi.getHostname().c_str());
#else
      setHostName(WiFi.hostname().c_str());
#endif
#endif
   }
   // silence the log messages on the console by default
   __nUsrLogLevel = 0;
#endif
   CxESPConsole::begin();
}

void CxESPConsole::begin() {
   println();println();
   info(F("==== CONSOLE ===="));

   if (__nUsers == __nMaxUsers) {
      warn(F("Connection will be closed, max. number of clients reached."));
      _abortClient();
   }
   __nUsers++;
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
}

void CxESPConsoleMaster::wlcm() {
   CxESPConsole::wlcm();
   _mapCapInstances["basic"]->execute("info");
}

void CxESPConsoleClient::wlcm() {
   CxESPConsole::wlcm();
   CxESPConsoleMaster::getInstance().processCmd(*__ioStream, "info");
}

bool CxESPConsole::hasFS() {
   return false;
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
         println();
/*
         print("heap: ");
#ifdef ARDUINO
         print(ESP.getFreeHeap());
#endif
         println();
 */
         CxESPConsoleMaster::getInstance().processCmd(*__ioStream, _szCmdBuffer);
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
         print(c); // Zeichen anzeigen
      }
   }
}


#ifndef ESP_CONSOLE_NOWIFI
void CxESPConsole::_abortClient() {
   if (!isWiFiClient()) {
      println("No exit on a serial connection.");
      return;
   }
#ifdef ARDUINO
   WiFiClient* client = reinterpret_cast<WiFiClient*>(__ioStream);
   
   if (client && client->connected()) {
      client->abort(); // abort WiFiClient
   }
#endif
}
#endif

void CxESPConsole::loop() {
   __handleConsoleInputs();
   __totalCPU.measureCPULoad();
}

void CxESPConsoleMaster::loop() {
   __sysCPU.stopMeasure();
   startMeasure();
   CxESPConsole::loop();

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
               processCmd(client, commandBuffer);
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
            __espConsoleWiFiClient = _createClientInstance(_activeClient, getAppName(), getAppVer()); // Neue Instanz mit WiFiClient
            if (__espConsoleWiFiClient) {
               __espConsoleWiFiClient->setHostName(getHostName());
               __espConsoleWiFiClient->begin();
            } else {
               error(F("*** error: _createInstance() for new wifi client failed!"));
            }
            g_Heap.update();
         } else if (__espConsoleWiFiClient) {
            info(F("Client disconnected."));
            delete __espConsoleWiFiClient;
            __espConsoleWiFiClient = nullptr;
            g_Heap.update();
         }
      }
      
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->loop(); // Befehle in der Hauptschleife abarbeiten
   }
#endif
#endif
   stopMeasure();
   for (auto& entry : _mapCapInstances) {
      entry.second->setIoStream(*__ioStream);
      entry.second->startMeasure();
      entry.second->loop();
      entry.second->stopMeasure();
   }
   __sysCPU.startMeasure();
}


bool CxESPConsoleMaster::isHostAvailable(const char* szHost, int nPort) {
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
   snprintf(buf, lenmax, "%s [%c] ", getTime(true), c);
   return (uint32_t)strlen(buf);
}

void CxESPConsole::debug(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('D', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   __debug(buf);
   
   va_end(args);
}

void CxESPConsole::debug(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('D', buf, sizeof(buf));
   
   vsnprintf_P(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __debug(buf);
   
   va_end(args);
}

void CxESPConsole::debug_ext(uint32_t flag, const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug_ext(flag, fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('X', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   __debug_ext(flag, buf);
      
   va_end(args);
   
}

void CxESPConsole::debug_ext(uint32_t flag, const FLASHSTRINGHELPER *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->debug_ext(flag, fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('X', buf, sizeof(buf));
   
   vsnprintf_P(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __debug_ext(flag, buf);
   
   va_end(args);
   
}

void CxESPConsole::info(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->info(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('I', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);
   
   __info(buf);
   
   va_end(args);
}

void CxESPConsole::info(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->info(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('I', buf, sizeof(buf));
   
   vsnprintf_P(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __info(buf);
   
   va_end(args);
}

void CxESPConsole::warn(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->warn(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('W', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);

   __warn(buf);
   
   va_end(args);
}

void CxESPConsole::warn(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->warn(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('W', buf, sizeof(buf));
   
   vsnprintf_P(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);
   
   __warn(buf);
   
   va_end(args);
}

void CxESPConsole::error(const char *fmt, ...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->error(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('E', buf, sizeof(buf));
   
   vsnprintf(buf+len, sizeof(buf)-len, fmt, args);

   __error(buf);
   
   va_end(args);
}

void CxESPConsole::error(const FLASHSTRINGHELPER * fmt...) {
   
   va_list args;
   va_start(args, fmt);
   
   if (!isWiFiClient()) {
      if (__espConsoleWiFiClient) __espConsoleWiFiClient->error(fmt, args); // forward to wifi client console
   }
   
   char buf[100];
   
   uint32_t len = _addPrefix('E', buf, sizeof(buf));
   
   vsnprintf_P(buf+len, sizeof(buf)-len, (PGM_P) fmt, args);

   __error(buf);
   
   va_end(args);
}
