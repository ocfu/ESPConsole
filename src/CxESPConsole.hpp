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
//#include "../capabilities/CxCapabilityBasic.hpp"

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
   
protected:
   bool __bIsWiFiClient = false;
   
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

public:
   explicit CxESPConsoleBase(Stream& stream) : __ioStream(&stream) {}
   CxESPConsoleBase() : __ioStream(nullptr) {}
   
   virtual ~CxESPConsoleBase() {}
   
   
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


};

class CxESPConsole : public CxESPConsoleBase, public CxESPTime, public CxProcessStatistic {
         
   String _strHostName; // WiFi.hostname() seems to be a messy workaround, even unable to find where it is defined in github... its return is a String and we can't trust that its c_str() remains valid. So we take a copy here.
   
   const char* _szUserName = "";
   const char* _szAppName = "";
   const char* _szAppVer = "";
   
   
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
   // logging functions
   uint32_t _addPrefix(char c, char* buf, uint32_t lenmax);
   
#ifndef ESP_CONSOLE_NOWIFI
   void _abortClient(); // aborts (ends) the (WiFi) client
#endif
   
protected:
   
   CxESPConsole* __espConsoleWiFiClient = nullptr;
   
   static uint8_t __nUsers;
   uint8_t __nMaxUsers = 2; // 1 serial, 1 wifi
   CxProcessStatistic __totalCPU;
   CxProcessStatistic __sysCPU;

   
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
   
   
   
   ///
   /// protected virtual methods
   ///
   protected:
   
   virtual void __prompt() {
      print(ESC_CLEAR_LINE);
      printf(FMT_PROMPT_DEFAULT);
   }
   
public:
   ///
   /// Constructor needed to differenciate between serial and wifi clients to abort the the session, if needed, properly.
   ///
#ifndef ESP_CONSOLE_NOWIFI
   CxESPConsole(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsole((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}
#endif
   CxESPConsole(Stream& stream, const char* app = "", const char* ver = "")
   : CxESPConsoleBase(stream), CxESPTime(), _nCmdHistorySize(4), _szAppName(app), _szAppVer(ver) {

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
      
      if (__nUsers) __nUsers--;
      
      ///
      /// release the allocated space for the command history buffer
      ///
      for (int i = 0; i < _nCmdHistorySize; ++i) {
         delete[] _aszCmdHistory[i];
      }
      delete[] _aszCmdHistory;
   }
      
   bool isWiFiClient() {return __bIsWiFiClient;}

   void setHostName(const char* sz) {_strHostName = sz;}
   const char* getHostNameForPrompt() {return isWiFiClient() ? (_strHostName.length() ? _strHostName.c_str() : "host") : "serial";}
   const char* getHostName() {return _strHostName.c_str();}
   const char* getUserName() {return _szUserName[0] ? _szUserName : "esp";}
   void setUserName(const char* sz) {_szUserName = sz;}

   void setAppNameVer(const char* szName, const char* szVer) {_szAppName = szName;_szAppVer = szVer;}
   const char* getAppName() {return _szAppName[0] ? _szAppName : "Arduino";}
   const char* getAppVer() {return _szAppVer[0] ? _szAppVer : "-";}

   uint8_t users() {return __nUsers;}

   
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


   virtual void begin();
   virtual void end(){}
   virtual void wlcm();
   virtual void loop();
   virtual bool hasFS();
   
   void cls() {print(F(ESC_CLEAR_SCREEN));}
      
   void setLogLevel(uint32_t set) {__nLogLevel = set;}
   uint32_t getLogLevel() {return __nLogLevel;}
   
   void setUsrLogLevel(uint32_t set) {__nUsrLogLevel = set;}
   uint32_t getUsrLogLevel() {return __nUsrLogLevel;}

   void setDebugFlag(uint32_t set) {__nExtDebugFlag = set;}
   void resetDebugFlag(uint32_t set) {__nExtDebugFlag &= ~set;}
   uint32_t getDebugFlag() {return __nExtDebugFlag;}

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
   
   void printUptimeExt() {
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
};

class CxESPConsoleClient : public CxESPConsole {
public:
   CxESPConsoleClient(WiFiClient& wifiClient, const char* app = "", const char* ver = "") : CxESPConsole((Stream&)wifiClient, app, ver) {__bIsWiFiClient = true;}

   virtual void begin() override {
      info(F("==== CLIENT ===="));
      
      CxESPConsole::begin();
   };
   
   virtual void loop() override {
      CxESPConsole::loop();
   };
   
   virtual void wlcm() override;

};


// Master console at serial port and manage capabilities
// Instance shall exist only once

class CxESPConsoleMaster : public CxESPConsole {
      
#ifndef ESP_CONSOLE_NOWIFI
   WiFiServer* _pWiFiServer = nullptr;
   WiFiClient _activeClient;
#endif
   
   std::map<String, std::unique_ptr<CxCapability> (*)(const char*)> _mapCapRegistry;  // Function pointers for constructors
   std::map<String, std::unique_ptr<CxCapability>> _mapCapInstances;  // Stores created instances
   
#ifndef ESP_CONSOLE_NOWIFI
    CxESPConsoleClient* _createClientInstance(WiFiClient& wifiClient, const char* app = "", const char* ver = "") const {
      return new CxESPConsoleClient(wifiClient, app, ver);
   }
#endif
   
   ~CxESPConsoleMaster() = default;// enforce singleton pattern
   
public:
   CxESPConsoleMaster() : CxESPConsole(Serial) {}
   
   // singleton pattern
   static CxESPConsoleMaster& getInstance() {
      static CxESPConsoleMaster instance;
      return instance;
   }
   
   // Disable copying and assignment
   CxESPConsoleMaster(const CxESPConsoleMaster&) = delete;
   CxESPConsoleMaster& operator=(const CxESPConsoleMaster&) = delete;

   virtual void begin() override;
   virtual void loop() override;

   // Register constructor method (Prevent duplicates)
   bool regCap(const char* name, std::unique_ptr<CxCapability> (*constructor)(const char*)) {
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
//   std::unique_ptr<CxCapability> createCapInstanceCpy(const char* name, const char* param) {
//      // If an instance already exists, return a new copy
//      auto itInstance = _mapCapInstances.find(name);
//      if (itInstance != _mapCapInstances.end()) {
//         return std::make_unique<CxCapability>(*itInstance->second);  // Copy existing instance
//      }
//      
//      // If a constructor exists, create and store the instance
//      auto it = _mapCapRegistry.find(name);
//      if (it != _mapCapRegistry.end()) {
//         std::unique_ptr<CxCapability> newInstance = it->second(param); // could be improved?
//         _mapCapInstances[name] = std::make_unique<CxCapability>(*newInstance);  // Store copy
//         return newInstance;  // Return newly created instance
//      }
//      print(F("Capability '")); print(name); println(F("' not copied."));
//      return nullptr;
//   }
   
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
         size_t mem = g_Heap.available(true); // force update
         std::unique_ptr<CxCapability> instance = it->second(name); // could be improved?
         if (instance) {
            instance->setIoStream(*__ioStream);
            instance->setup();
            _mapCapInstances[name] = std::move(instance); // don't use instance any more after std::move !!
            _mapCapInstances[name].get()->setMemAllocation(mem - g_Heap.available(true));
            print(F("Capability '" ESC_ATTR_BOLD)); print(name); print(F(ESC_ATTR_RESET "' loaded. " ESC_ATTR_BOLD));
            print(_mapCapInstances[name].get()->getMemAllocation()); print(F(ESC_ATTR_RESET" bytes allocated. " ESC_ATTR_BOLD));
            print(_mapCapInstances[name].get()->getCommandsCount()); println(F(ESC_ATTR_RESET" commands added."));
         } else {
            print(F("Capability '")); print(name); println(F("' could not be created."));
            return nullptr;
         }
         return _mapCapInstances[name].get();
      }
      print(F("Capability '")); print(name); println(F("' not found."));
      return nullptr;
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
   
   // process (loop) statistics
   void printPs() {
      println(F(ESC_ATTR_BOLD "Name     Cmd  Time Load Avg" ESC_ATTR_RESET));
      printf( "%-8s ", "sys");
      print(F("*    "));
      printf("%4d ", __sysCPU.looptime());
      printf("%1.2f ", __sysCPU.load());
      printf("%1.2f", __sysCPU.avgload());
      println();
      
      printf("%-8s ", "cons");
      print(F("loop "));
      printf("%4d ", looptime());
      printf("%1.2f ", load());
      printf("%1.2f", avgload());
      println();

      for (const auto& entry : _mapCapRegistry) {
         if (_mapCapInstances.find(entry.first) != _mapCapInstances.end()) {
            printf("%-8s ", entry.first.c_str());
            print(F("loop "));
            printf("%4d ", _mapCapInstances[entry.first].get()->looptime());
            printf("%1.2f ", _mapCapInstances[entry.first].get()->load());
            printf("%1.2f", _mapCapInstances[entry.first].get()->avgload());
            println();
         }
      }

      printf(ESC_ATTR_BOLD "%-8s ", "total");
      print(F("*    "));
      printf("%4d ", __totalCPU.looptime());
      printf("%1.2f ", __totalCPU.load());
      printf("%1.2f", __totalCPU.avgload());
      println(ESC_ATTR_RESET);

   }

   
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
   
   bool processCmd(const char* cmd);
   bool processCmd(Stream& stream, const char* cmd) {
      // redirect stream to client
      Stream* pStream = __ioStream;
      __ioStream = &stream;
      
      bool bResult = processCmd(cmd);
      
      __ioStream = pStream;
      
      return bResult;
   }
   
   
#ifndef ESP_CONSOLE_NOWIFI
   void begin(WiFiServer& server) {
      _pWiFiServer = &server;
      server.begin();
      begin();
   };
#endif
   
   virtual void wlcm() override;

   
   //virtual void saveEnv(String& strEnv, String& strValue) {};
   //virtual bool loadEnv(String& strEnv, String& strValue) {return false;};


};




#endif /* CxESPConsole_hpp */
