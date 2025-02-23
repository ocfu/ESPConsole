//
//  CxCapability.hpp
//  ESP
//
//  Created by ocfu on 09.01.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#ifndef CxCapability_hpp
#define CxCapability_hpp


#include "Arduino.h"

// include some generic defines, such as ESC sequences, format for prompts, debug macros etc.
#include "defines.h"

#ifndef ARDUINO
#include "devenv.h"
#endif

#include <vector>
#include <map>
#include <memory>

#define CXCAPABILITY(className, capName, ...) \
class className : public CxCapability { \
public: \
   explicit className() \
      : CxCapability(capName, getCmds()) {} \
   static constexpr const char* getName() { return capName; } \
   static const std::vector<const char*>& getCmds() { \
      static std::vector<const char*> commands = { __VA_ARGS__ }; \
      return commands; \
   } \
   static std::unique_ptr<CxCapability> construct(const char* param) { \
      return std::make_unique<className>(); \
   } \
   virtual void setup() override; \
   virtual void loop() override; \
   virtual bool execute(const char* cmd, const char* args) override; \
}; \

class CxCapability : public Print {
   Stream* _ioStream = nullptr;

protected:
   bool __bLocked;
   size_t __nMemAllocation = 0;

   const char* name;  // Command set name
   
   std::vector<const char*> commands;  // List of commands (e.g., "reboot", "start", "pause")
   
   virtual const std::vector<const char*>& getCommands() {return commands;}

public:
   explicit CxCapability(const char* setName, const std::vector<const char*>& cmds) : name(setName?setName:"unknown"), commands(cmds), __bLocked(false) {
   }
   virtual ~CxCapability() {}
   
   bool isLocked() {return __bLocked;}
   size_t getMemAllocation() {return __nMemAllocation;}
   void setMemAllocation(size_t set) {__nMemAllocation = set;}
   uint32_t getCommandsCount() {return (uint32_t)commands.size();}
   
   void setIoStream(Stream& stream) {_ioStream = &stream;}
   Stream& getIoStream() {
      if (_ioStream) {
         return *_ioStream;
      } else {
         return Serial;
      }
   }
   
   const char* getName() { return name;}
   
   virtual void setup() {}
   virtual void loop() {}
   virtual bool execute(const char* cmd, const char* args) {return false;}
   
   virtual size_t write(uint8_t c) override;
   virtual size_t write(const uint8_t *buffer, size_t size) override;
   
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
   
   bool processCmd(const char* cmd);
   
   // Function to print all commands ordered by setName
   void printCommands() {
      // Sort the commands alphabetically
      std::sort(commands.begin(), commands.end(), [](const char* a, const char* b) {
         return strcmp(a, b) < 0;
      });
      
      // Print the command set name
      printf(ESC_ATTR_BOLD "%s: " ESC_ATTR_RESET ESC_TEXT_BRIGHT_WHITE, name);
      
      // Print each command
      int i = 0;
      for (const auto& cmd : commands) {
         if (i++ > 0) print(", ");
         print(cmd);
      }
      println(ESC_ATTR_RESET);
   }
};


#endif /* CxCapability */

