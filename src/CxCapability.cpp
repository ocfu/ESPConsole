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

uint8_t CxCapability::processCmd(const char* szCmdLine, uint8_t nClient) {
   if (!szCmdLine) return EXIT_FAILURE;  // No command found
   return execute(szCmdLine, nClient);
}


