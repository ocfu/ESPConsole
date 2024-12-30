//
//  esphw.h
//  xESP
//
//  Created by ocfu on 14.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef esphw_h
#define esphw_h

#include "Arduino.h"

size_t getStackSize();
void reboot();
void factoryReset();
bool isExceptionRestart();
const char* getFreeHeap();
uint32_t getFreeOTA();
const char* getResetReason();
const char* getResetInfo();
const char* getHeapFragmentation();
const char* getChipInfo();
const char* getChipType();
uint32_t getChipId();
const char* getCoreVersion();
void printEspInfo(Stream& stream);
void printEspFlashMap(Stream& stream);
bool utf8_check_is_valid(const char* sz);
uint64_t speed_check(uint32_t start = 1, uint32_t end = 1023);
char* remove8BitChars(const char *mess);
void replaceInvalidChars(char * sz, uint32_t lenmax);
uint32_t getFlashChipSize();
uint32_t getFlashChipSize();
uint32_t getFlashChipRealSize();

#endif /* esphw_h */
