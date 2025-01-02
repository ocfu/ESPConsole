//
//  esphw.cpp
//  xESP
//
//  Created by ocfu on 14.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "Arduino.h"
#ifndef ARDUINO
#include "devenv.h"
#endif
#include <stdio.h>
#include <math.h>

char __nst_buffer[255];


// ESP8266 stuff here ////////////////////////////////////////////////////////
#ifndef ESP32

#ifdef ARDUINO
extern "C" {
#include "c_types.h"
#include "spi_flash.h"
#include "user_interface.h"
}
extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end; // Usually points to 0x405FB000
extern "C" uint32_t _EEPROM_start;
#endif  // ARDUINO


// https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map


/* Flash Split for 1M chips */
/* sketch @0x40200000 (~935KB) (958448B) */
/* empty  @0x402E9FF0 (~4KB) (4112B) */
/* spiffs @0x402EB000 (~64KB) (65536B) */
/* eeprom @0x402FB000 (4KB) */
/* rfcal  @0x402FC000 (4KB) */
/* wifi   @0x402FD000 (12KB) */

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define SKETCH_START 0x40200000


#define FLASHFS_START  ((uint32_t) (&_SPIFFS_start))
#define FLASHFS_END    ((uint32_t) (&_SPIFFS_end))

#define EPROM_START  0x402FB000
#define EPROM_END    (EPROM_START + 0x1000)

#define RFCAL_START   0x402FC000
#define RFCAL_END     (RFCAL_START + 0x3000)

#define WIFI_START   0x402FD000
#define WIFI_END     (WIFI_START + 0x3000)

#define CHIP_DETECT_MAGIC_REG_ADDR             0x40001000  // This ROM address has a different value on each chip model
#define GET_UINT32(addr) *((volatile uint32_t *)((addr)))

#define OTA_END       (MIN(FLASHFS_START, EPROM_START)  - 0x1)
#define OTA_START     (OTA_END - ESP.getSketchSize())
#define FREE_END      (OTA_START - 0x1)
#define FREE_START    (SKETCH_START + ESP.getSketchSize())
#define FREE_SIZE     (FREE_END - FREE_START)

#ifdef ARDUINO
const char * const FLASH_SIZE_MAP_NAMES[] = {
   "1Mbits_MAP_256kBytes_256kBytes",
   "2Mbits",
   "8Mbits_MAP_512kBytes_512kBytes",
   "16Mbits_MAP_512kBytes_512kBytes",
   "32Mbits_MAP_512kBytes_512kBytes",
   "16Mbits_MAP_1024kBytes_1024kBytes",
   "32Mbits_MAP_1024kBytes_1024kBytes"
};
#endif

//Credits
//http://www.bitflippin.net/riscv/bfref/esp8266/
//https://github.com/espressif/esptool/blob/master/esptool.py
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/index.html
//https://gitlab.unizar.es/761347/esp-idf/blob/5122154dbbbce0c4339c3aa7864b06d3707c1350/components/soc/esp32/include/soc/soc.h

#define BIT(nr)                 (1UL << (nr))
#define BIT31   0x80000000
#define BIT30   0x40000000
#define BIT29   0x20000000
#define BIT28   0x10000000
#define BIT27   0x08000000
#define BIT26   0x04000000
#define BIT25   0x02000000
#define BIT24   0x01000000
#define BIT23   0x00800000
#define BIT22   0x00400000
#define BIT21   0x00200000
#define BIT20   0x00100000
#define BIT19   0x00080000
#define BIT18   0x00040000
#define BIT17   0x00020000
#define BIT16   0x00010000
#define BIT15   0x00008000
#define BIT14   0x00004000
#define BIT13   0x00002000
#define BIT12   0x00001000
#define BIT11   0x00000800
#define BIT10   0x00000400
#define BIT9     0x00000200
#define BIT8     0x00000100
#define BIT7     0x00000080
#define BIT6     0x00000040
#define BIT5     0x00000020
#define BIT4     0x00000010
#define BIT3     0x00000008
#define BIT2     0x00000004
#define BIT1     0x00000002
#define BIT0     0x00000001

//get bit or get bits from uint32
#define GET_BIT(_a, _b)  ((_a) & (_b))

//set bit or set bits to uint32
#define SET_BIT(_a, _b)  ((_a) |= (_b))


// esp8266/65 128 bits efuse
#define DR_REG_EFUSE_BASE 0x3ff00050

struct efuse_esp82xx {
   uint32_t _r0;   //   0..31
   uint32_t _r1;   //  32..63
   uint32_t _r2;   //  64..95
   uint32_t _r3;   //  96..127
};

volatile efuse_esp82xx* _pefuse_esp82xx = (volatile efuse_esp82xx*)(0x3ff00050);


int get_flash_size_esp82xx() {
   uint32_t r0_4     = GET_BIT(_pefuse_esp82xx->_r0, BIT4);
   uint32_t r3_25    = GET_BIT(_pefuse_esp82xx->_r3, BIT25);
   uint32_t r3_26    = GET_BIT(_pefuse_esp82xx->_r3, BIT26);
   uint32_t r3_27    = GET_BIT(_pefuse_esp82xx->_r3, BIT27);
   
   if (r0_4 && !r3_25) {
      if (!r3_27 && !r3_26) {
         return 1;
      } else if (!r3_27 && r3_26) {
         return 2;
      }
   }
   
   if (!r0_4 && r3_25) {
      if (!r3_27 && !r3_26) {
         return 2;
      } else if (!r3_27 && r3_26) {
         return 4;
      }
   }
   return -1;
}



bool is_8285() {
   return (GET_BIT(_pefuse_esp82xx->_r0, BIT4) || GET_BIT(_pefuse_esp82xx->_r2, BIT16));
}

const char* get_chip_type_esp82xx() {
   uint32_t r0_4  = GET_BIT(_pefuse_esp82xx->_r0, BIT4);
   uint32_t r0_5  = GET_BIT(_pefuse_esp82xx->_r0, BIT5);  // max. temperatur bit
   uint32_t r2_16 = GET_BIT(_pefuse_esp82xx->_r2, BIT16);
   
   if (r0_4 || r2_16) {
      // chip is from type esp8285 if on of these bits is set
      // determine further the specific esp8285 type
      int flash_size = get_flash_size_esp82xx();
      
      switch(flash_size) {
         case 1:
            return (r0_5 != 0) ? "ESP8285H08" : "ESP8285N08";
            break;
         case 2:
            return (r0_5 != 0) ? "ESP8285H16" : "ESP8285N16";
            break;
         default:
            return "ESP8285";
      }
   }
   return "ESP8266EX";
}

#define MAX_UINT32 0xffffffff
#define MAX_UINT24 0xffffff

#define ESP_OTP_MAC0    0x3ff00050
#define ESP_OTP_MAC1    0x3ff00054

uint32_t get_chip_id_esp82xx() {
   // """ Read Chip ID from efuse - the equivalent of the SDK system_get_chip_id() function """
   uint32_t id0 = GET_UINT32(ESP_OTP_MAC0);
   uint32_t id1 = GET_UINT32(ESP_OTP_MAC1);
   return (id0 >> 24) | ((id1 & MAX_UINT24) << 8);
}

//#ifdef ARDUINO
const char* getChipType() {
   static char buf[20];
   
   switch (GET_UINT32(CHIP_DETECT_MAGIC_REG_ADDR)) {
      case 0xfff0c101:
         strncpy(buf, get_chip_type_esp82xx(), sizeof(buf)-1);
         break;
      case 0x00f01d83:
         strncpy_P(buf, (PGM_P)F("ESP32"), sizeof(buf)-1);
         break;
      case 0x000007c6:
         strncpy_P(buf, (PGM_P)F("ESP32-S2"), sizeof(buf)-1);
         break;
      case 0xeb004136:
         strncpy_P(buf, (PGM_P)F("ESP32-S3-BETA2"), sizeof(buf)-1);
         break;
      case 0x00000009:
         strncpy_P(buf, (PGM_P)F("ESP32-S3-BETA3"), sizeof(buf)-1);
         break;
      case 0x6921506f:
         strncpy_P(buf, (PGM_P)F("ESP32C3-ECO12"), sizeof(buf)-1);
         break;
      case 0x1b31506f:
         strncpy_P(buf, (PGM_P)F("ESP32C3-ECO3"), sizeof(buf)-1);
         break;
      case 0x0da1806f:
         strncpy_P(buf, (PGM_P)F("ESP32C6-BETA"), sizeof(buf)-1);
         break;
      default:
         snprintf_P(buf, sizeof(buf)-1, (PGM_P)F("UNKNOWN (0x%X)"), GET_UINT32(CHIP_DETECT_MAGIC_REG_ADDR));
         break;
   }
   buf[sizeof(buf)-1] = '\0';
   return buf;
   
}

#else   // ESP32 stuff from here ///////////////////////////////////////////////
#include "esp_partition.h"
#endif // ESP32


// Common ESP8266 and ESP32 stuff here /////////////////////////////////////////////////

uint64_t speed_check(uint32_t start, uint32_t end) {
   
   //not working yet
   
   uint32_t fnd = 0;
   uint32_t ctr = 0;
   uint64_t now = millis();
   
   for (uint32_t i = start; i <= end; i++) {
      for (uint32_t j = 2; j <= sqrt(i); j++) {
         if (i % j == 0)
            ctr++;
      }
      
      if (ctr == 0 && i != 1) {
         fnd++;
         ctr=0;
#ifdef ARDUINO
         yield();
#endif
      }
      ctr=0;
   }
   return (millis() - now);
}

uint32_t getFreeOTA() {
#ifdef ARDUINO
#ifdef ESP32
   return 0;
#else
   return (FREE_SIZE);
#endif
#else
   return 0;
#endif
}

uint32_t getChipId() {
#ifdef ESP32
   uint32_t id = 0;
   for(int i=0; i<17; i=i+8) {
      id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
   }
   return id;
#else
#ifdef ARDUINO
   return ESP.getChipId();
#else
   return 0xAAFFAA;
#endif
#endif
}

char* remove8BitChars(const char *mess) {
   static char __buffer[80];
   
   int i = 0, decmp = 0;
   
   for (; mess[i] && decmp < (sizeof(__buffer)-1); i++ )
      if ((unsigned char)mess[i] <= 127 || (unsigned char)mess[i] == 176 /*degree sign*/)
         __buffer[decmp++] = mess[i];
   __buffer[decmp] = 0;
   return __buffer;
}

void replaceInvalidChars(char* sz, uint32_t lenmax) {
   while (*sz != '\0' && lenmax-- > 0) {
      // Prüfen, ob das Zeichen ein Buchstabe, eine Zahl oder ein Leerzeichen ist
      if (!isalnum((unsigned char)*sz) && !isspace((unsigned char)*sz)) {
         *sz = '-';  // Ersetzen des ungültigen Zeichens
      }
      sz++;  // Zum nächsten Zeichen in der Zeichenkette gehen
   }
}

bool utf8_check_is_valid(const char* sz)
{
   int c,i,n,j;
   size_t ix;
   
   for (i=0, ix=strlen(sz); i < ix; i++)
   {
      c = (unsigned char) sz[i];
      //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
      if (0x00 <= c && c <= 0x7f) n=0; // 0bbbbbbb
      else if ((c & 0xE0) == 0xC0) n=1; // 110bbbbb
      else if ( c==0xed && i<(ix-1) && ((unsigned char)sz[i+1] & 0xa0)==0xa0) return false; //U+d800 to U+dfff
      else if ((c & 0xF0) == 0xE0) n=2; // 1110bbbb
      else if ((c & 0xF8) == 0xF0) n=3; // 11110bbb
      //else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
      //else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
      else return false;
      for (j=0; j<n && i<ix; j++) { // n bytes matching 10bbbbbb follow ?
         if ((++i == ix) || (( (unsigned char)sz[i] & 0xC0) != 0x80))
            return false;
      }
   }
   return true;
}

const char* getChipInfo() {
#ifdef ARDUINO
#ifdef ESP32
   snprintf(__nst_buffer, sizeof(__nst_buffer), "ESP32x");
#else
   snprintf(__nst_buffer, sizeof(__nst_buffer), "%s/%dMHz/%dM", getChipType(), ESP.getCpuFreqMHz(), ESP.getFlashChipRealSize()/0x100000);
#endif
#else
   __nst_buffer[0] = '\0';
#endif
   __nst_buffer[sizeof(__nst_buffer)-1] = '\0';
   return __nst_buffer;
}


const char* getResetReason() {
#ifdef ARDUINO
#ifdef ESP32
   //TODO: implement reset reason for esp32 cpu0+1
   snprintf(__nst_buffer, sizeof(__nst_buffer), "%d", -1);
#else
   snprintf(__nst_buffer, sizeof(__nst_buffer), "%s", ESP.getResetReason().c_str());
#endif
#else
   __nst_buffer[0] = '\0';
#endif
   __nst_buffer[sizeof(__nst_buffer)-1] = '\0';
   return __nst_buffer;
}


const char* getResetInfo() {
#ifdef ARDUINO
#ifdef ESP32
   //TODO: implement reset info for esp32 cpu0+1
   snprintf(__nst_buffer, sizeof(__nst_buffer), "%d", -1);
#else
   rst_info * resetInfo = ESP.getResetInfoPtr();
   
   if (resetInfo != nullptr && resetInfo->exccause > 0) {
      snprintf(__nst_buffer, sizeof(__nst_buffer), "### Exception: %s", ESP.getResetInfo().c_str());
   } else {
      snprintf(__nst_buffer, sizeof(__nst_buffer), "%s", ESP.getResetReason().c_str());
   }
#endif
#else
   snprintf(__nst_buffer, sizeof(__nst_buffer), "Restart");
#endif
   __nst_buffer[sizeof(__nst_buffer)-1] = '\0';
   return __nst_buffer;
}

bool isExceptionRestart() {
#ifdef ARDUINO
#ifdef ESP32
   //TODO: implement reset info for esp32 cpu0+1
#else
   rst_info * resetInfo = ESP.getResetInfoPtr();
   
   if (resetInfo != nullptr && resetInfo->exccause > 0) {
      return true;
   }
#endif
#endif
   return false;
}

const char* getCoreVersion() {
#ifdef ARDUINO
#ifdef ESP32
   snprintf(__nst_buffer, sizeof(__nst_buffer), "%s", ESP.getCoreVersion().c_str());
#else
   snprintf(__nst_buffer, sizeof(__nst_buffer), "%s", ESP.getCoreVersion().c_str());
#endif
#else
   __nst_buffer[0] = '\0';
#endif
   __nst_buffer[sizeof(__nst_buffer)-1] = '\0';
   return __nst_buffer;
}

uint32_t getFlashChipSize() {
#ifdef ARDUINO
#ifdef ESP32
   return ESP.getFlashChipSize();
#else
   return ESP.getFlashChipSize();
#endif
#else
   return 0;
#endif
}

uint32_t getFlashChipRealSize() {
#ifdef ARDUINO
#ifdef ESP32
   return ESP.getFlashChipSize();
#else
   return ESP.getFlashChipRealSize();
#endif
#else
   return 0;
#endif
}

const char* getMapName() {
#ifdef ARDUINO
   return FLASH_SIZE_MAP_NAMES[system_get_flash_size_map()];
#else
   return "";
#endif
}

uint32_t getFreeSize() {
#ifdef ARDUINO
   return FREE_SIZE;
#else
   return 0;
#endif
}

uint32_t getFSSize() {
#ifdef ARDUINO
   return FLASHFS_END - FLASHFS_START;
#else
   return 0;
#endif
}

uint32_t getSketchStart() {
#ifdef ARDUINO
   return SKETCH_START;
#else
   return 0;
#endif
}

uint32_t getFreeStart() {
#ifdef ARDUINO
   return FREE_START;
#else
   return 0;
#endif
}

uint32_t getFreeEnd() {
#ifdef ARDUINO
   return FREE_END;
#else
   return 0;
#endif
}

uint32_t getOTAStart() {
#ifdef ARDUINO
   return OTA_START;
#else
   return 0;
#endif
}

uint32_t getOTAEnd() {
#ifdef ARDUINO
   return OTA_END;
#else
   return 0;
#endif
}

uint32_t getFlashFSStart() {
#ifdef ARDUINO
   return FLASHFS_START;
#else
   return 0;
#endif
}

uint32_t getFlashFSEnd() {
#ifdef ARDUINO
   return FLASHFS_END;
#else
   return 0;
#endif
}

uint32_t getEPROMStart() {
#ifdef ARDUINO
   return EPROM_START;
#else
   return 0;
#endif
}

uint32_t getEPROMEEnd() {
#ifdef ARDUINO
   return EPROM_END;
#else
   return 0;
#endif
}

uint32_t getRFCALStart() {
#ifdef ARDUINO
   return RFCAL_START;
#else
   return 0;
#endif
}

uint32_t getRFCALEnd() {
#ifdef ARDUINO
   return RFCAL_END;
#else
   return 0;
#endif
}

uint32_t getWIFIStart() {
#ifdef ARDUINO
   return WIFI_START;
#else
   return 0;
#endif
}

uint32_t getWIFIEnd() {
#ifdef ARDUINO
   return WIFI_END;
#else
   return 0;
#endif
}
