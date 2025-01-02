//
//  CxESPConsole.hpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPConsole_hpp
#define CxESPConsole_hpp

#include "Arduino.h"

#include "../tools/CxESPHeapTracker.hpp"
#include "../tools/CxESPTime.hpp"
#include "../tools/CxStrToken.hpp"

#ifdef ARDUINO
#ifndef ESP_CONSOLE_NOWIFI
  #include <WiFiClient.h>
  #ifdef ESP32
    #include <WiFi.h>
  #else  // not ESP32
    #include <ESP8266WiFi.h>
  #endif // end ESP32
#endif
  #define FLASHSTRINGHELPER __FlashStringHelper

#else //  not ARDUINO
 #include "devenv.h"
#endif // end ARDUINO

// include some generic defines, such as ESC sequences, format for prompts etc.
#include "defines.h"




///
/// CxESPConsoleBase class
/// pure virtual base class to force instanziation of the virtual __processCommand
///
class CxESPConsoleBase  {
protected:
   virtual bool __processCommand(const char* szCmd, bool bQuiet = false) = 0;
};

///
/// CxESPConsole class
/// Provides a command console for the ESP on the serial port,
/// using "Serial" and opens a WiFi server on a port and waits for
/// connects of WiFiClients. The clients will be provided with the
/// command console.
/// CxESPConsole is also the base class for further specialised
/// consoles, to provide additional capabilities.
///
class CxESPConsole : public CxESPConsoleBase, public CxESPTime {

   ///
   /// public typedefs
   ///
public:
   

   ///
   /// public constructors and destructors
   ///
public:
   ///
   /// Constructor needed to differenciate between serial and wifi clients to abort the the session, if needed, properly.
   ///
#ifndef ESP_CONSOLE_NOWIFI
   CxESPConsole(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsole((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
#endif
   CxESPConsole(Stream& stream, const char* app = "", const char* ver = "")
   : CxESPTime(stream), __ioStream(&stream), _nCmdHistorySize(4), _szAppName(app), _szAppVer(ver) {
      
      _nLastMeasurement = (uint32_t) micros(); // Startzeit initialisieren
   
      // Zeit für die aktive Aufgabe messen
      _nStartActive = (uint32_t) micros();

      ///
      /// allocate space for the command history buffer with a history up to <size>
      /// the max. length of each command line is determined by _nMAXLENGTH
      ///
      _aszCmdHistory = new char*[_nCmdHistorySize];
      for (int i = 0; i < _nCmdHistorySize; ++i) {
         _aszCmdHistory[i] = new char[_nMAXLENGTH]();
      }
   }
   
   
   // virtual destructor as the object might be created by a inherited class
   virtual ~CxESPConsole() {
      if (_nUsers) _nUsers--;
      
      ///
      /// release the allocated space for the command history buffer
      ///
      for (int i = 0; i < _nCmdHistorySize; ++i) {
         delete[] _aszCmdHistory[i];
      }
      delete[] _aszCmdHistory;
   }
   
   ///
   /// public methods
   ///
public:
   void setHostName(const char* sz) {_strHostName = sz;}
   const char* getHostNameForPrompt() {return _isWiFiClient() ? (_strHostName.length() ? _strHostName.c_str() : "host") : "serial";}
   const char* getHostName() {return _strHostName.c_str();}
   void setAppNameVer(const char* szName, const char* szVer) {_szAppName = szName;_szAppVer = szVer;}
   const char* getAppName() {return _szAppName[0] ? _szAppName : "Arduino";}
   const char* getAppVer() {return _szAppVer[0] ? _szAppVer : "-";}
   void setUserName(const char* sz) {_szUserName = sz;}
   const char* getUserName() {return _szUserName[0] ? _szUserName : "esp";}
   
   bool isConnected() {
#ifdef ARDUINO
      return (WiFi.status() == WL_CONNECTED);
#else
      return false;
#endif
   }

   ///
   /// print to stream methods
   ///
   
   void printf(const char* sz...); // limited to 127 chars (stream class has no vprintf)
   void printf(const FLASHSTRINGHELPER * szP...);
   void printf() {printf("");}
   
   void println(const char* sz) {__ioStream->println(sz);}
   void println(const FLASHSTRINGHELPER * szP) {/*__ioStream->println((PGM_P)szP);*/ __ioStream->printf((PGM_P)szP);println();}
   void println() {println("");} // WiFiClient->println chrashes with flash strings
   
   void print(const char* sz) {__ioStream->print(sz);}
   void print(const FLASHSTRINGHELPER * szP) {/*__ioStream->print((PGM_P)szP);*/ __ioStream->printf((PGM_P)szP);} // WiFiClient->print chrashes with flash strings
   void print() {print("");}
   void print(const char c){__ioStream->print(c);}

   void printUptimeExt();

   void printHeap();
   void printHeapAvailable(bool fmt = false);
   void printHeapSize(bool fmt = false);
   void printHeapUsed(bool fmt = false);
#ifndef ESP_CONSOLE_NOWIFI
   void printHostName();
   void printIp();
   void printSSID();
#endif
   
   // TODO: get number of connected clients
   uint8_t users() {return _nUsers;}
   float load() {return _fLoad;}
   float avgload() {return _fAvgLoad;}
   uint32_t avglooptime() {return _navgLoopTime;}

   ///
   /// public virtual methods
   ///
public:
#ifndef ESP_CONSOLE_NOWIFI
   void begin(WiFiServer& server) {_pWiFiServer = &server;begin();};
#endif
   virtual void begin();
   virtual void end(){}
   virtual void wlcm();
   virtual void loop();
   virtual bool hasFS();
   
   void cls() {print(F(ESC_CLEAR_SCREEN));}
   
   virtual void printInfo();

   ///
   /// protected members and methods
   ///
protected:
   Stream* __ioStream;                   // Pointer to the stream object (serial or WiFiClient)
   bool __bIsWiFiClient = false;

   const char* __szConsoleName = ""; // appears at the start message
   void setConsoleName(const char* sz) {
      if (!__szConsoleName[0]) __szConsoleName = sz; // set only, if not set by calling class already
   }

   void __handleConsoleInputs();
   void __flush() {__ioStream->flush();}
   
   ///
   /// protected virtual methods
   ///
protected:   
   virtual bool __processCommand(const char* szCmd, bool bQuiet = false) override;
   
   virtual void __prompt() {
      __ioStream->print(ESC_CLEAR_LINE);
      __ioStream->printf(FMT_PROMPT_DEFAULT);
   }
   
   ///
   /// User response queries and handling
   /// Each usr query function must be non-blocking, means the process will not stop to wait for the answer.
   /// This will be realsied with a state _bWaitingForUsrResponse and a query related callback function.
   ///
   
   ///
   /// Yes/No user response query
   /// - Parameter message: message presented at the prompt
   /// - Parameter callback: callback function to the reaction
   void __promptUserYN(const char* message, void (*responseCallback)(bool)) {
      __ioStream->print(ESC_CLEAR_LINE);
      __ioStream->printf(FMT_PROMPT_USER_YN, message);
      __ioStream->print(" \b");        // position the cursor behind the command
      _bWaitingForUsrResponseYN = true;
      _cbUsrResponse = responseCallback;
   }
   
   ///
   /// private virtual methods
   ///
private:

   ///
   /// private members
   ///
private:
#ifndef ESP_CONSOLE_NOWIFI
   WiFiServer* _pWiFiServer = nullptr;
   WiFiClient activeClient;
   CxESPConsole* espConsoleWiFiClient = nullptr;
#endif
   
   String _strHostName; // WiFi.hostname() seems to be a messy workaround, even unable to find where it is defined in github... its return is a String and we can't trust that its c_str() remains valid. So we take a copy here.
   
   const char* _szAppName = "";
   const char* _szAppVer = "";
   const char* _szUserName = "";
   static uint8_t _nUsers;
   uint8_t _nMaxUsers = 2; // 1 serial, 1 wifi
   
   static const int _nMAXLENGTH = 64;   // Max. command line length
   char _szCmdBuffer[_nMAXLENGTH];      // Command line buffer
   int _iCmdBufferIndex = 0;            // Actual cursor position in command line (always at the last char of input, cur left/right not supported)
   
   char** _aszCmdHistory;               // Command line history buffer
   int _nCmdHistorySize;                // History size (max. number of command lines in the buffer)
   int _nCmdHistoryCount = 0;           // Actual number of command lines in the buffer
   int _iCmdHistoryIndex = -1;          // Acutal index of the command line buffer
   int _nStateEscSequence = 0;          // Actual ESC sequence state during input
   
   bool _bWaitingForUsrResponseYN = false;   // Indicates an active (pending) user response
   void (*_cbUsrResponse)(bool) = nullptr; // Callback for the response answer
   
   uint32_t _nLastMeasurement = 0; // Zeitmarke des letzten Messzyklus
   uint32_t _nActiveTime = 0;      // Aktive Zeit für den aktuellen Zyklus
   uint32_t _nTotalTime = 1000000; // Beobachtungszeitraum pro Zyklus (1 Sekunde in Mikrosekunden)
   uint32_t _nLoops = 0;
   uint32_t _navgLoopTime = 0;
   
   // Variablen für Durchschnitt
   uint32_t _nTotalActiveTime = 0; // Gesamte aktive Zeit über alle Zyklen
   uint32_t _nTotalObservationTime = 0; // Gesamtzeit über alle Zyklen
   uint32_t _nStartActive = 0;
   float _fAvgLoad = 0.0;
   float _fLoad = 0.0;
   
   ///
   /// private methods
   ///
private:
#ifndef ESP_CONSOLE_NOWIFI
   ///
   /// Will be used when a new client connects (even in derived class objects). A new ESPConsole instance will be created  in
   /// the loop of the base class.   This is the reason, why the destructor must be virtual, otherwies resources of the derived
   /// class would not be de-allocated.
   ///
   virtual CxESPConsole* _createInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const {
      return new CxESPConsole(wifiClient, app, ver);
   }
#endif

   void _clearCmdBuffer() {
      memset(_szCmdBuffer, 0, _nMAXLENGTH);
      _iCmdBufferIndex = 0;
   }
   
   void _storeCmd(const char* command) {
      if (strlen(command) == 0) return; // No storage of empty commands
      
      // Check, if the command is the same as the previous, than do not store it in the command buffer
      if (_nCmdHistoryCount > 0) {
         int lastCommandIndex = (_nCmdHistoryCount - 1) % _nCmdHistorySize;
         if (strcmp(_aszCmdHistory[lastCommandIndex], command) == 0) {
            return;
         }
      }
      
      // Store the new command in command history buffer
      strncpy(_aszCmdHistory[_nCmdHistoryCount % _nCmdHistorySize], command, _nMAXLENGTH);
      _nCmdHistoryCount++;
   }
   
   void _navigateCmdHistory(int direction) {
      if (_nCmdHistoryCount == 0) return; // no history of commands yet
      
      // calc the next index for the command history buffer
      int newIndex = _iCmdHistoryIndex + direction;
      int maxValidIndex = std::min(_nCmdHistoryCount, _nCmdHistorySize) - 1;
      
      // check boundaries
      if (newIndex < -1 || newIndex > maxValidIndex) {
         return; // out of boundary
      }
      
      _iCmdHistoryIndex = newIndex;
      
      if (_iCmdHistoryIndex == -1) {
         _clearCmdBuffer(); // clear current command, nothing left in the history and show prompt
         __prompt();
      } else {
         // restore command from command history buffer
         strncpy(_szCmdBuffer, _aszCmdHistory[(_nCmdHistoryCount - 1 - _iCmdHistoryIndex) % _nCmdHistorySize], _nMAXLENGTH);
         _iCmdBufferIndex = (int)strlen(_szCmdBuffer);
         _redrawCmd();
      }
   }
      
   void _redrawCmd() {
      __prompt();
      __ioStream->print(_szCmdBuffer); // output the command from the buffer
      __ioStream->print(" \b");        // position the cursor behind the command
   }
         
   void _handleUserResponse(char c) {
      if (_bWaitingForUsrResponseYN) {
         if (c == 'y' || c == 'Y') {
            __ioStream->printf("Yes");
            _bWaitingForUsrResponseYN = false;
            if (_cbUsrResponse) _cbUsrResponse(true);
         } else if (c == 'n' || c == 'N') {
            __ioStream->println("No");
            _bWaitingForUsrResponseYN = false;
            if (_cbUsrResponse) _cbUsrResponse(false);
         } else {
            __ioStream->println("Invalid input. Please type 'y' or 'n'.");
         }
         println();
      }
   }
#ifndef ESP_CONSOLE_NOWIFI
   void _abortClient(); // aborts (ends) the (WiFi) client
#endif
   bool _isWiFiClient() {return __bIsWiFiClient;}
   
   void _measureCPULoad();
   
};




#endif /* CxESPConsole_hpp */
