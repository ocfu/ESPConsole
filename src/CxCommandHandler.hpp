//
//  CxCommandHandler.hpp
//
//  Created by ocfu on 01.02.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#ifndef CxCommandHandler_hpp
#define CxCommandHandler_hpp

#include "Arduino.h"

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <cstring>

#ifdef ARDUINO
#ifdef ESP32
#else  // no ESP32
#endif // end ESP32
#else
#include "devenv.h"
#endif // end ARDUINO


///
/// CxCommandHandler
///
/// @brief Singleton class for handling command registration and execution.
///
/// This class allows registering command sets and processing commands received from a stream.
/// Commands are stored in a map and can be executed via function handlers.
/// It also supports grouping commands under categories for better organization.
///
/// Command sets require memory allocation. Depending on the application, it may be preferable
/// to store them in RAM or PROGMEM. Both options are supported. However, when using PROGMEM,
/// dynamic memory allocation is required to copy strings to RAM.
///
/// Pros of using PROGMEM:
/// - Saves generally RAM, which is limited on microcontrollers.
/// - Reduces memory footprint when storing large command sets.
///
/// Cons of using PROGMEM:
/// - Requires dynamic memory allocation, which can lead to fragmentation.
/// - Increases processing time due to additional memory operations.
///
/// Why Using PROGMEM?
///   Saves Static RAM at Compile Time:
///   - Storing command strings directly in RAM would consume valuable space on microcontrollers.
///   - Keeping them in PROGMEM avoids bloating RAM with unused commands before they are needed.
///
///   Larger Programs Possible:
///   - Flash memory is usually much larger than available RAM on embedded systems.
///   - By storing command sets in PROGMEM, more complex applications can fit within hardware limits.
///
/// Why Does It Still Use RAM?
///   String Operations Require RAM:
///   - Standard C functions (strcmp, strtok, snprintf, etc.) don’t work directly with PROGMEM.
///   - This forces dynamic memory allocation to copy PROGMEM strings into RAM before use.
///
///   Temporary RAM Usage:
///   - While commands are copied into RAM, they only persist in dynamically allocated memory, not in permanent global/static RAM.
///   - This means RAM is only used when needed rather than being preoccupied at compile time.
///
/// Trade-offs
///   - If you have sufficient RAM: Storing command sets directly in RAM avoids dynamic memory allocation and fragmentation.
///   - If RAM is limited: Using PROGMEM prevents unnecessary RAM usage at compile time but requires runtime allocation.
///
/// In short, using PROGMEM is beneficial when RAM is a scarce resource, even though a temporary RAM copy is needed for processing.
/// If RAM usage isn’t a concern, storing command sets in RAM directly is simpler and avoids allocation overhead.
/// Recommendation: use RAM, as the commandHandler instance will be created in any case at program start, thus will
/// allocate space for the command sets in RAM anyhow.
///
/// Room for improvement:
/// TODO: dynamically load command sets, when needed.
/// TODO: unregister method.
///
class CxCommandHandler {
private:
   /// @brief Comparator for sorting C-string keys in maps.
   struct StrCompare {
      bool operator()(const char* a, const char* b) const {
         return strcmp(a, b) < 0;
      }
   };
   
   /// @brief Represents an entry in the group map, containing description and list of commands.
   struct GroupEntry {
      std::function<bool(const char*, bool bQuiet)> handler; ///< Function to execute the command.
      const char* description; ///< Description of the command group.
      std::vector<const char*> commands; ///< List of commands in this group.
   };
   
   /// @brief Copies a PROGMEM string to a dynamically allocated buffer in RAM.
   /// @param progmemStr String stored in PROGMEM.
   /// @return Pointer to dynamically allocated string in RAM.
   ///
   /// Flash strings require copying into RAM before usage since string operations
   /// like `strcmp`, `strtok`, and `snprintf` do not work directly on PROGMEM data.
   ///
   char* strdup_P(const char* progmemStr) {
      size_t len = strlen_P(progmemStr) + 1;
      char* ramStr = (char*)malloc(len);
      if (ramStr) {
         strcpy_P(ramStr, progmemStr);
      }
      return ramStr;
   }
   
   std::map<const char*, GroupEntry, StrCompare> groupMap; ///< Map storing command groups and descriptions.
   
   /// @brief Private constructor for singleton pattern.
   CxCommandHandler() = default;

public:
   /// @brief Deleted copy constructor to prevent duplication.
   CxCommandHandler(const CxCommandHandler&) = delete;
   
   /// @brief Deleted assignment operator to prevent duplication.
   CxCommandHandler& operator=(const CxCommandHandler&) = delete;
   
   /// @brief Retrieves the singleton instance of the command handler.
   /// @return Reference to the singleton instance.
   static CxCommandHandler& getInstance() {
      static CxCommandHandler instance;
      return instance;
   }
   
   /// @brief Registers a command set under a specific group.
   /// @param groupName Name of the command group.
   /// @param handler Function that will be executed when a command is invoked.
   /// @param commandsHelp Comma-separated list of commands.
   /// @param groupDescription Description of the command group.
   void registerCommandSet(const char* groupName,
                           std::function<bool(const char*, bool bQuiet)> handler,
                           const char* commandsHelp,
                           const char* groupDescription) {
      std::vector<const char*> commandList;
      char* helpCopy = strdup(commandsHelp);
      char* token = strtok(helpCopy, ", ");
      
      while (token != nullptr) {
         const char* cmd = strdup(token);
         commandList.push_back(cmd);
         //commandMap[cmd] = {handler, groupName};
         token = strtok(nullptr, ", ");
      }
      free(helpCopy);
      
      groupMap[groupName] = {handler, groupDescription, commandList};
   }
   
   /// @brief Registers a command set using flash strings stored in PROGMEM.
   /// @param groupName Group name stored in PROGMEM.
   /// @param handler Function handler for commands.
   /// @param commandsHelp Commands stored in PROGMEM.
   /// @param groupDescription Group description stored in PROGMEM.
   ///
   /// This overload exists because flash strings cannot be directly used in
   /// string-processing functions without first copying them into RAM.
   ///
   void registerCommandSet(const __FlashStringHelper* groupName,
                           std::function<bool(const char*, bool)> handler,
                           const __FlashStringHelper* commandsHelp,
                           const __FlashStringHelper* groupDescription) {
      char* groupNameBuf = strdup_P((PGM_P)groupName);
      char* commandsHelpBuf = strdup_P((PGM_P)commandsHelp);
      char* groupDescBuf = strdup_P((PGM_P)groupDescription);
      
      registerCommandSet(groupNameBuf, handler, commandsHelpBuf, groupDescBuf);
   }

   /// @brief Processes a received command from a stream.
   /// @param stream Output stream to print responses.
   /// @param commandLine The command input string.
   /// @param bQuiet If true, suppresses output messages.
   ///
   /// This method is necessary for handling commands dynamically entered
   /// via a serial or wifi interface. It extracts the command and executes the
   /// associated handler if found.
   ///
   void processCommand(Stream& stream, const char* commandLine, bool bQuiet = false) {
      if (!commandLine) {
         return;
      }
      
      bool bFound = false;
      for (const auto& [groupName, group] : groupMap) {
         bFound = group.handler(commandLine, bQuiet);
         if (bFound) break;
      }
   }
   
   /// @brief Prints help information for all registered commands and groups.
   /// @param stream Output stream to print the help information.
   void printHelp(Stream& stream) {
      for (const auto& [groupName, group] : groupMap) {
         stream.printf(ESC_ATTR_BOLD "%s: " ESC_ATTR_RESET ESC_TEXT_BRIGHT_WHITE, group.description);
         for (size_t i = 0; i < group.commands.size(); ++i) {
            if (i > 0) stream.print(", ");
            stream.print(group.commands[i]);
         }
         stream.println(ESC_ATTR_RESET);
      }
   }
};

#endif /* CxCommandHandler_hpp */
