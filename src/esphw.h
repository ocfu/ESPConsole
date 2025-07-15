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
#ifdef ARDUINO
#include <EEPROM.h>
#ifdef ESP_CONSOLE_WIFI
#include <WiFiClient.h>
#ifdef ESP32
#include <WiFi.h>
#else  // not ESP32
#include <ESP8266WiFi.h>
#endif // end ESP32
#endif
#else
#include "devenv.h"
#endif

// additional settings hosted in eeprom at 0x100
typedef struct s_settings {

   uint32_t _loopDelay;
   
   s_settings() : _loopDelay(0) {}
} Settings_t;


void readSettings(Settings_t& settings);
void writeSettings(Settings_t& settings);

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
bool utf8_check_is_valid(const char* sz);
uint64_t speed_check(uint32_t start = 1, uint32_t end = 1023);
char* remove8BitChars(const char *mess);
void replaceInvalidChars(char * sz, uint32_t lenmax);
uint32_t getFlashChipSize();
uint32_t getFlashChipRealSize();
bool is_8285();
const char* getMapName();
uint32_t getFSSize();
uint32_t getSketchStart();
uint32_t getOTAStart();
uint32_t getOTAEnd();
uint32_t getFlashFSStart();
uint32_t getFlashFSEnd();
uint32_t getEPROMStart();
uint32_t getEPROMEEnd();
uint32_t getRFCALStart();
uint32_t getRFCALEnd();
uint32_t getWIFIStart();
uint32_t getWIFIEnd();
void printEEPROM(Stream& stream, uint32_t nStartAddr = 0, uint32_t nLength = 4096);
bool readSSID(char* szSSID, uint32_t lenmax);
bool writeSSID(const char* szSSID);
bool readPassword(char* szPassword, uint32_t lenmax);
bool writePassword(const char* szPassword);
bool readHostName(char* szHostname, uint32_t lenmax);
bool writeHostName(const char* szHostname);
bool readOtaPassword(char* szPassword, uint32_t lenmax);
bool writeOtaPassword(const char* szPassword);

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
   unsigned int i;
#ifdef ARDUINO
   const byte* p = (const byte*)(const void*)&value;
   for (i = 0; i < sizeof(value); i++)
      EEPROM.write(ee++, *p++);
#endif
   return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
   unsigned int i;
#ifdef ARDUINO
   byte* p = (byte*)(void*)&value;
   for (i = 0; i < sizeof(value); i++)
      *p++ = EEPROM.read(ee++);
#endif
   return i;
}

template <class T> int EEPROM_vanishData(int ee, T& value)
{
   unsigned int i;
#ifdef ARDUINO
   for (i = 0; i < sizeof(value); i++)
      EEPROM.write(ee++, '\0');
#endif
   return i;
}

void scanWiFi(Stream& stream);


#endif /* esphw_h */
