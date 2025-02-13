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

// include some generic defines, such as ESC sequences, format for prompts, debug macros etc.
#include "defines.h"

#include "CxCapability.hpp"

#include "../tools/CxESPHeapTracker.hpp"
#include "../tools/CxESPTime.hpp"
#include "../tools/CxStrToken.hpp"
#include "../tools/CxTimer.hpp"

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

#include <map>
#include <vector>


///
/// CxESPConsoleBase class
/// pure virtual base class to force instanziation of the virtual __processCommand
///
/// and to implement print methods.
///
class CxESPConsoleBase : public Print  {
   
   std::map<String, std::unique_ptr<CxCapability> (*)(CxESPConsole&, const char*)> _mapCapRegistry;  // Function pointers for constructors
   std::map<String, std::unique_ptr<CxCapability>> _mapCapInstances;  // Stores created instances

public:
   explicit CxESPConsoleBase(Stream& stream) : __ioStream(&stream) {}
   CxESPConsoleBase() : __ioStream(nullptr) {}
   
   virtual ~CxESPConsoleBase() {}
   
   // Register constructor method (Prevent duplicates)
   bool regCap(const char* name, std::unique_ptr<CxCapability> (*constructor)(CxESPConsole&, const char*)) {
      if (_mapCapRegistry.find(name) != _mapCapRegistry.end()) {
         print(F("Capability '")); print(name); println(F("' already listed."));
         return false;  // Registration failed
      }
      _mapCapRegistry[name] = constructor;
      return true;  // Registration successful
   }
   
   // Unregister a constructor method and remove instance
   void unregCap(const char* name) {
      _mapCapRegistry.erase(name);
      _mapCapInstances.erase(name);
   }
   
   // Create an instance or return existing one (copy pointer)
   std::unique_ptr<CxCapability> createCapInstanceCpy(const char* name, const char* param) {
      // If an instance already exists, return a new copy
      auto itInstance = _mapCapInstances.find(name);
      if (itInstance != _mapCapInstances.end()) {
         return std::make_unique<CxCapability>(*itInstance->second);  // Copy existing instance
      }
      
      // If a constructor exists, create and store the instance
      auto it = _mapCapRegistry.find(name);
      if (it != _mapCapRegistry.end()) {
         std::unique_ptr<CxCapability> newInstance = it->second(*((CxESPConsole*) this), param); // could be improved?
         _mapCapInstances[name] = std::make_unique<CxCapability>(*newInstance);  // Store copy
         return newInstance;  // Return newly created instance
      }
      
      return nullptr;  // Constructor not found
   }
   
   // Create an instance or return existing one
   CxCapability* createCapInstance(const char* name, const char* param) {
      // If an instance already exists, return the same pointer
      auto itInstance = _mapCapInstances.find(name);
      if (itInstance != _mapCapInstances.end()) {
         print(F("Capability '")); print(name); println(F("' already exists."));
         return itInstance->second.get();
      }
      
      // If a constructor exists, create and store the instance
      auto it = _mapCapRegistry.find(name);
      if (it != _mapCapRegistry.end()) {
         size_t mem = g_Heap.available();
         std::unique_ptr<CxCapability> instance = it->second(*((CxESPConsole*) this), name); // could be improved?
         if (instance) {
            instance->setup();
            _mapCapInstances[name] = std::move(instance); // don't use instance any more after std::move !!
            _mapCapInstances[name].get()->setMemAllocation(mem - g_Heap.available());
            print(F("Capability '" ESC_ATTR_BOLD)); print(name); print(F(ESC_ATTR_RESET "' loaded. " ESC_ATTR_BOLD));
            print(_mapCapInstances[name].get()->getMemAllocation()); print(F(ESC_ATTR_RESET" bytes allocated. " ESC_ATTR_BOLD));
            print(_mapCapInstances[name].get()->getCommandsCount()); println(F(ESC_ATTR_RESET" commands added."));
         } else {
            return nullptr;
         }
         return _mapCapInstances[name].get();
      }
      print(F("Capability '")); print(name); println(F("' not found."));
      return nullptr;  // Constructor not found
   }
   
   void deleteCapInstance(const char* name) {
      auto it = _mapCapInstances.find(name);
      if (it != _mapCapInstances.end()) {
         if (!it->second.get()->isLocked()) {
            _mapCapInstances.erase(it);  // Unique_ptr automatically deletes the object
            print(F("Capability '")); print(name); println(F("' deleted."));
         } else {
            print(F("Capability '")); print(name); println(F("' is locked!"));
         }
      } else {
         print(F("Capability '")); print(name); println(F("' not found."));
      }
   }
   
   // Print all registered constructors
   void listCap() {
      println(F("Available Capabilities: "));
      for (const auto& entry : _mapCapRegistry) {
         print("- "); print(entry.first.c_str());
         if (_mapCapInstances.find(entry.first) != _mapCapInstances.end()) {
            print(F(" (loaded"));
            if (_mapCapInstances[entry.first].get()->isLocked()) {
               print(F(" and locked"));
            }
            print(F(", "));
            print(_mapCapInstances[entry.first].get()->getMemAllocation());
            print(F(" bytes allocated, "));
            print(_mapCapInstances[entry.first].get()->getCommandsCount());
            print(F(" commands)"));
            println();
         } else {
            println();
         }
      }
   }
   
   bool processCmd(const char* cmd);
   
   void setStream(Stream& stream) {__ioStream = &stream;}
   Stream* getStream() {return __ioStream;}
      
   void flush() {__ioStream->flush();}

   // Implement the required write function
   virtual size_t write(uint8_t c) override {
      if(__ioStream) {
         __ioStream->write(c);
         return 1;
      } else {
         return 0;
      }
   }
   
   // Optional: Override write() for string buffers (better efficiency)
   virtual size_t write(const uint8_t *buffer, size_t size) override {
      if (__ioStream) {
         return __ioStream->write(buffer, size);
      } else {
         return 0;
      }
   }

protected:
   
   Stream* __ioStream;                   // Pointer to the stream object (serial or WiFiClient)

   // Universal printf() that supports both Flash and RAM strings
   void printf(const char *format, ...) {
      char buffer[128];  // Temporary buffer for formatted string
      va_list args;
      va_start(args, format);
      vsnprintf(buffer, sizeof(buffer), format, args);
      va_end(args);
      print(buffer);  // Use Print's built-in print()
   }
   
   // Overloaded printf() for Flash (PROGMEM) strings
   void printf(const __FlashStringHelper *format, ...) {
      char buffer[128];  // Temporary buffer for formatted string
      va_list args;
      va_start(args, format);
      vsnprintf_P(buffer, sizeof(buffer), (PGM_P)format, args);
      va_end(args);
      print(buffer);  // Use Print's built-in print()
   }
   
   virtual void __debug(const char* sz) {println(sz);}
   virtual void __debug_ext(uint32_t flang, const char* sz) {println(sz);}
   virtual void __info(const char* sz) {println(sz);}
   virtual void __warn(const char* sz) {println(sz);}
   virtual void __error(const char* sz) {println(sz);}
   

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
   : CxESPConsoleBase(stream), CxESPTime(), _nCmdHistorySize(4), _szAppName(app), _szAppVer(ver) {
      
      regCap(CxCapabilityBasic::getName(), CxCapabilityBasic::construct);
      
      createCapInstance(CxCapabilityBasic::getName(), "");

      if (!_pESPConsoleInstance) _pESPConsoleInstance = this;
      
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
   static CxESPConsole* getInstance() {return _pESPConsoleInstance;}
   void setHostName(const char* sz) {_strHostName = sz;}
   const char* getHostNameForPrompt() {return __isWiFiClient() ? (_strHostName.length() ? _strHostName.c_str() : "host") : "serial";}
   const char* getHostName() {return _strHostName.c_str();}
   void setAppNameVer(const char* szName, const char* szVer) {_szAppName = szName;_szAppVer = szVer;}
   const char* getAppName() {return _szAppName[0] ? _szAppName : "Arduino";}
   const char* getAppVer() {return _szAppVer[0] ? _szAppVer : "-";}
   void setUserName(const char* sz) {_szUserName = sz;}
   const char* getUserName() {return _szUserName[0] ? _szUserName : "esp";}
   
   void reboot();

#ifndef ESP_CONSOLE_NOWIFI
   bool isConnected() {
#ifdef ARDUINO
      return (WiFi.status() == WL_CONNECTED);
#else
      return false;
#endif
   }
   bool isHostAvailable(const char* szHost, int nPort);
#endif

   void printUptimeExt();

   void printHeap();
   void printHeapAvailable(bool fmt = false);
   void printHeapSize(bool fmt = false);
   void printHeapUsed(bool fmt = false);
   void printHeapFragmentation(bool fmt = false);
#ifndef ESP_CONSOLE_NOWIFI
   void printHostName();
   void printIp();
   void printSSID();
   void printMode();
#endif
   
   void printProgress(uint32_t actual, uint32_t max, const char* header, const char* unit) {
      uint32_t progress = (actual * 100) / max;
      printf("\r\033[K%16s: %d%% (%d / %d %s)", header, progress, actual, max, unit);
   }
   
   void printProgressBar(uint32_t actual, uint32_t max, const char* header) {
      uint32_t progress = (actual * 100) / max;
      const uint8_t barWidth = 50; // Breite des Fortschrittsbalkens
      uint8_t pos = (progress * barWidth) / 100;
      
      printf("\r\033[K%16s: [", header);
      for (int i = 0; i < barWidth; i++) {
         if (i < pos) print("#");
         else print("-");
      }
      printf("] %d%%", progress);
   }
   
   // TODO: get number of connected clients
   uint8_t users() {return _nUsers;}
   float load() {return _fLoad;}
   float avgload() {return _fAvgLoad;}
   uint32_t avglooptime() {return _navgLoopTime;}
      
   // basic logging functions
   void debug(const char* fmt...);
   void debug(String& str) {debug(str.c_str());}
   void debug(const FLASHSTRINGHELPER * fmt...);
   
   void debug_ext(uint32_t flag, const char* fmt...);
   void debug_ext(uint32_t flag, String& str) {debug_ext(flag, str.c_str());}
   void debug_ext(uint32_t flag, const FLASHSTRINGHELPER * fmt...);
   
   void info(const char* fmt...);
   void info(String& str) {info(str.c_str());}
   void info(const FLASHSTRINGHELPER * fmt...);
   
   void warn(const char* fmt...);
   void warn(String& str) {warn(str.c_str());}
   void warn(const FLASHSTRINGHELPER * szP...);
   
   void error(const char* fmt...);
   void error(String& str) {error(str.c_str());}
   void error(const FLASHSTRINGHELPER * fmt...);


   ///
   /// public virtual methods
   ///
public:
#ifndef ESP_CONSOLE_NOWIFI
   void begin(WiFiServer& server) {
      _pWiFiServer = &server;
      server.begin();
      begin();
   };
#endif
   virtual void begin();
   virtual void end(){}
   virtual void wlcm();
   virtual void loop();
   virtual bool hasFS();
   
   void cls() {print(F(ESC_CLEAR_SCREEN));}
   
   virtual void printInfo();
   
   virtual void saveEnv(String& strEnv, String& strValue) {};
   virtual bool loadEnv(String& strEnv, String& strValue) {return false;};
   
   void setLogLevel(uint32_t set) {__nLogLevel = set;}
   uint32_t getLogLevel() {return __nLogLevel;}
   
   void setUsrLogLevel(uint32_t set) {__nUsrLogLevel = set;}
   uint32_t getUsrLogLevel() {return __nUsrLogLevel;}

   void setDebugFlag(uint32_t set) {__nExtDebugFlag = set;}
   void resetDebugFlag(uint32_t set) {__nExtDebugFlag &= ~set;}
   uint32_t getDebugFlag() {return __nExtDebugFlag;}

   ///
   /// protected members and methods
   ///
protected:
   bool __bIsWiFiClient = false;
   
   CxESPConsole* __espConsoleWiFiClient = nullptr;

   const char* __szConsoleName = ""; // appears at the start message
   void setConsoleName(const char* sz) {
      if (!__szConsoleName[0]) __szConsoleName = sz; // set only, if not set by calling class already
   }
   
   ///
   /// log levels
   ///  0: off
   ///  1: error
   ///  2: warning
   ///  3: info
   ///  4: debug
   ///  5: ext. debug (controlled by _nDebugFlag)
   ///
   uint32_t __nLogLevel  = 4;
   uint32_t __nUsrLogLevel = 4;
   
   ///
   /// debug flags 32 bits 0xfffffffe (except: -1)
   /// 0x0: off
   ///
   ///
   uint32_t __nExtDebugFlag = 0x0;

   void __handleConsoleInputs();
   
   bool __isWiFiClient() {return __bIsWiFiClient;}

   
   ///
   /// protected virtual methods
   ///
protected:   
   
   virtual void __prompt() {
      print(ESC_CLEAR_LINE);
      printf(FMT_PROMPT_DEFAULT);
   }
   
   ///
   /// User response queries and handling
   /// Each usr query function must be non-blocking, means the process will not stop to wait for the answer.
   /// This will be realsied with a state _bWaitingForUsrResponse and a query related callback function.
   ///

public:
   ///
   /// Yes/No user response query
   /// - Parameter message: message presented at the prompt
   /// - Parameter callback: callback function to the reaction
   void __promptUserYN(const char* message, void (*responseCallback)(bool)) {
      print(ESC_CLEAR_LINE);
      printf(FMT_PROMPT_USER_YN, message);
      print(" \b");        // position the cursor behind the command
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
   static CxESPConsole* _pESPConsoleInstance;
   
#ifndef ESP_CONSOLE_NOWIFI
   WiFiServer* _pWiFiServer = nullptr;
   WiFiClient _activeClient;
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
      print(_szCmdBuffer); // output the command from the buffer
      print(" \b");        // position the cursor behind the command
   }
         
   void _handleUserResponse(char c) {
      if (_bWaitingForUsrResponseYN) {
         if (c == 'y' || c == 'Y') {
            printf("Yes");
            _bWaitingForUsrResponseYN = false;
            if (_cbUsrResponse) _cbUsrResponse(true);
         } else if (c == 'n' || c == 'N') {
            println("No");
            _bWaitingForUsrResponseYN = false;
            if (_cbUsrResponse) _cbUsrResponse(false);
         } else {
            println("Invalid input. Please type 'y' or 'n'.");
         }
         println();
      }
   }
#ifndef ESP_CONSOLE_NOWIFI
   void _abortClient(); // aborts (ends) the (WiFi) client
#endif
   
   void _measureCPULoad();
   
   // logging functions
   uint32_t _addPrefix(char c, char* buf, uint32_t lenmax);

   
};




#endif /* CxESPConsole_hpp */
