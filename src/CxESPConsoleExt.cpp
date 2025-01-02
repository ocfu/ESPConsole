//
//  CxESPConsoleExt.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleExt.hpp"
#include "esphw.h"

void CxESPConsoleExt::begin() {
   // set the name for this console
   setConsoleName("Ext");
   
   // call the begin() from base class(es) first
   CxESPConsole::begin();

   // no specifics for this console

}

bool CxESPConsoleExt::__processCommand(const char *szCmd) {
   // validate the call
   if (!szCmd) return false;
   
   // get the command and arguments into the token buffer
   CxStrToken tkCmd(szCmd, " ");
   
   // validate again
   if (!tkCmd.count()) return false;
   
   // we have a command, find the action to take
   String cmd = TKTOCHAR(tkCmd, 0);
   
   // removes heading and trailing white spaces
   cmd.trim();
   
   // expect sz parameter, invalid is nullptr
   const char* a = TKTOCHAR(tkCmd, 1);
   const char* b = TKTOCHAR(tkCmd, 2);

   // expect integer parameter, invalid is -1
   int32_t x = TKTOINT(tkCmd, 1, -1);
   
   
   if (cmd == "?" || cmd == USR_CMD_HELP) {
      // show help first from base class(es)
      CxESPConsole::__processCommand(szCmd);
      println(F("Ext commands:" ESC_TEXT_BRIGHT_WHITE "     hw, sw, flash, esp" ESC_ATTR_RESET));
   } else if (cmd == "hw") {
      printHW();
   } else if (cmd == "sw") {
      printSW();
   } else if (cmd == "esp") {
      printESP();
   } else {
      // command not handled here, proceed into the base class(es)
      return CxESPConsole::__processCommand(szCmd);
   }
   return true;
}

void CxESPConsoleExt::printHW() {
   printf(F(ESC_ATTR_BOLD "    Chip Type:" ESC_ATTR_RESET " %s " ESC_ATTR_BOLD "Chip-ID: " ESC_ATTR_RESET "0x%X\n"), getChipType(), getChipId());
#ifdef ARDUINO
   printf(F(ESC_ATTR_BOLD "   Flash Size:" ESC_ATTR_RESET " %dk (real) %dk (ide)\n"), getFlashChipRealSize()/1024, getFlashChipSize()/1024);
   printf(F(ESC_ATTR_BOLD "Chip-Frequenz:" ESC_ATTR_RESET " %dMHz\n"), ESP.getCpuFreqMHz());
#endif
}

void CxESPConsoleExt::printSW() {
#ifdef ARDUINO
   printf(F(ESC_ATTR_BOLD "   Plattform:" ESC_ATTR_RESET " %s"), ARDUINO_BOARD);
   printf(F(ESC_ATTR_BOLD " Core Ver.:" ESC_ATTR_RESET " %s\n"), ESP.getCoreVersion().c_str());
   printf(F(ESC_ATTR_BOLD "    SDK Ver.:" ESC_ATTR_RESET " %s\n"), ESP.getSdkVersion());
   

#ifdef ARDUINO_CLI_VER
   int arduinoVersion = ARDUINO_CLI_VER;
   const char* ide="(cli)";
#else
   int arduinoVersion = ARDUINO;
   const char* ide="(ide)";
#endif
   int major = arduinoVersion / 10000;
   int minor = (arduinoVersion / 100) % 100;
   int patch = arduinoVersion % 100;
   printf(F(ESC_ATTR_BOLD "Arduino Ver.:" ESC_ATTR_RESET " %d.%d.%d %s\n"), major, minor, patch, ide);
#endif
   if (getAppName()[0]) printf(F(ESC_ATTR_BOLD "    Firmware:" ESC_ATTR_RESET " %s Ver.:" ESC_ATTR_RESET " %s\n"), getAppName(), getAppVer());
}

/*
void printNetworkInfo(Stream& stream) {
   stream.printf(F("Net:  %s (%c)"),  Eprom.getAPSTAName(), Wifi1.isConnected()?'*':'X');
   stream.printf(F("RSSI: %d"), WiFi1.getRSSI());
   stream.printf(F("Host: %s"), Wifi1.getHostname());
   stream.printf(F("IP:   %s"), Wifi1.getIP());
   stream.printf(F("GW:   %s"), Wifi1.getGateway());
   stream.printf(F("DNS:  %s (%s)"), Wifi1.getDNS(), Wifi1.isHostAvailble(Wifi1.getGateway(), 53)?"available":"not avail.");
   Log.info(XC_F("NTP:  %s"), Time1.getNtpServer());
}
*/

void CxESPConsoleExt::printInfo() {
   CxESPConsole::printInfo();
   printf(F(ESC_ATTR_BOLD   "      Chip: " ESC_ATTR_RESET "%s " ESC_ATTR_BOLD "Sw:" ESC_ATTR_RESET " %s\n"), getChipInfo(), _strCoreSdkVersion.c_str());

}

void CxESPConsoleExt::printESP() {
#ifdef ARDUINO
#ifdef ESP32
   //TODO: get real flash size for esp32
   uint32_t realSize = ESP.getFlashChipSize();
#else
   uint32_t realSize = ESP.getFlashChipRealSize();
#endif
   uint32_t ideSize = ESP.getFlashChipSize();
   FlashMode_t ideMode = ESP.getFlashChipMode();
   
   printf(F("-CPU--------------------\n"));
#ifdef ESP32
   printf(F("ESP:          %s\n"), "ESP32");
#else
   printf(F("ESP:          %s\n"), getChipType());
#endif
   printf(F("Freq:         %d MHz\n"), ESP.getCpuFreqMHz());
   printf(F("ChipId:       %X\n"), getChipId());
   printf(F("MAC:          %s\n"), WiFi.macAddress().c_str());
   printf(F("\n"));
#ifdef ESP32
   printf(F("-FLASH------------------\n"));
#else
   if (is_8285()) {
      printf(F("-FLASH-(embeded)--------\n"));
   } else {
      printf(F("-FLASH------------------\n"));
   }
#endif
#ifdef ESP32
   printf(F("Vendor:       unknown\n"));
#else
   printf(F("Vendor:       0x%X\n"), ESP.getFlashChipVendorId());  // complete list in spi_vendors.h
#ifdef PUYA_SUPPORT
   if (ESP.getFlashChipVendorId() == SPI_FLASH_VENDOR_PUYA) printf(F("Puya support: Yes\n"));
#else
   printf(F("Puya support: No\n"));
   if (ESP.getFlashChipVendorId() == SPI_FLASH_VENDOR_PUYA) {
      printf(F("WARNING: #### vendor is PUYA, FLASHFS will fail, if you don't define -DPUYA_SUPPORT (ref. esp8266/Arduino #6221)\n"));
   }
#endif
#endif
   printf(F("Size (real):  %d kBytes\n"), realSize/1024);
   printf(F("Size (comp.): %d kBytes\n"), ideSize/1024);
   if(realSize != ideSize) {
      printf(F("### compiled size differs from real chip size\n"));
   }
   //printf(F("CRC ok:       %d\n"),ESP.checkFlashCRC());
   printf(F("Freq:         %d MHz\n"), ESP.getFlashChipSpeed()/1000000);
   printf(F("Mode (ide):   %s\n"), ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN");
#ifdef ESP32
   printf(F("Size Map:     unknown\n"));
#else
   printf(F("Size Map:     %s\n"), getMapName());
#endif
   printf(F("Size avail.:  %7d Bytes\n"), ESP.getSketchSize() + ESP.getFreeSketchSpace());
   printf(F("     sketch:  %7d Bytes\n"), ESP.getSketchSize());
   printf(F("       free:  %7d Bytes\n"), ESP.getFreeSketchSpace());
#ifdef ESP32
   printf(F("   fr.w.OTA:  ? Bytes\n"));
#else
   printf(F("   fr.w.OTA:  %7d Bytes\n"), getFreeSize());
   if (getFreeSize() < 20000) {
      printf(F("*** Free size for OTA very low!\n"));
   } else if (getFreeSize() < 100000) {
      printf(F("*** Free size for OTA is getting low!\n"));
   }
   printf(F("FLASHFS size: %6d Bytes\n"), getFSSize());
#endif
   printf(F("\n"));
   printf(F("-FIRMWARE---------------\n"));
#ifdef ESP32
   //TODO: implement esp core version for esp32
   printf(F("ESP core:     unknown\n"));
#else
   printf(F("ESP core:     %s\n"), ESP.getCoreVersion().c_str());
#endif
   printf(F("ESP sdk:      %s\n"), ESP.getSdkVersion());
   printf(F("Application:  %s (%s)\n"), getAppName(), getAppVer());
   printf(F("\n"));
   printf(F("-BOOT-------------------\n"));
   printf(F("reset reason: %s\n"), getResetInfo());
   print(F("time to boot: ")); printTimeToBoot(); println();
   printf(F("free heap:    %5d Bytes\n"), ESP.getFreeHeap());
   printf(F("\n"));
#endif
}
