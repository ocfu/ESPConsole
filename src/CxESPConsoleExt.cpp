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
