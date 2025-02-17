//
//  CxCapability.cpp
//  ESP
//
//  Created by ocfu on 09.01.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

size_t CxCapability::write(uint8_t c) {
   if (_ioStream) {
      return _ioStream->write(c);
   }
   return 0;
}

size_t CxCapability::write(const uint8_t *buffer, size_t size) {
   if (_ioStream) {
      return _ioStream->write(buffer, size);
   }
   return 0;
}

bool CxCapability::processCmd(const char* szCmdLine) {
   char buffer[64];  // Copy the command for strtok
   strncpy(buffer, szCmdLine, sizeof(buffer) - 1);
   buffer[sizeof(buffer) - 1] = '\0';
   
   char* command = strtok(buffer, " ");  // Get the main command
   char* args = strtok(nullptr, "");     // Get the remaining arguments
   
   if (!command) return false;  // No command found
   
   for (const auto& cmdInSet : commands) {
      if (strcmp(command, cmdInSet) == 0) {
         if (execute(command, args ? args : "")) return true;
      }
   }
   return false;
}


