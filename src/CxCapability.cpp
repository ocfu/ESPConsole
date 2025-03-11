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
   if (!szCmdLine) return false;  // No command found
   
   if (*szCmdLine == '$' || *szCmdLine == '.') { // hidden command
      return execute(szCmdLine);
   } else {
      if (execute(szCmdLine)) return true;
   }
   return false;
}


