//
//  CxESPConsoleExt.cpp
//  xESP
//
//  Created by ocfu on 11.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#include "CxESPConsoleExt.hpp"
#include "../tools/CxGpio.hpp"
#include "esphw.h"

#ifndef ESP_CONSOLE_NOWIFI
#include "../tools/CxOta.hpp"
#ifdef ARDUINO
#ifdef ESP32
#include <WebServer.h>
WebServer webServer(80);
#else
#include <ESP8266WebServer.h>
ESP8266WebServer webServer(80);
#endif /* ESP32*/
#include <DNSServer.h>
DNSServer dnsServer;
const byte DNS_PORT = 53;

#endif /* ARDUINO */

// HTML and CSS as embedded strings
const char htmlPageTemplate[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WiFi Setup</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f9;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }
    .container {
      text-align: center;
      background: white;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      width: 300px;
    }
    h1 {
      margin-bottom: 20px;
      font-size: 24px;
    }
    form {
      display: flex;
      flex-direction: column;
    }
    label {
      margin-bottom: 5px;
      text-align: left;
    }
    select, input {
      margin-bottom: 15px;
      padding: 8px;
      border: 1px solid #ccc;
      border-radius: 5px;
      width: 100%;
    }
    button {
      background-color: #007bff;
      color: white;
      padding: 10px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    button:hover {
      background-color: #0056b3;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>WiFi Setup</h1>
    <form action="/connect" method="POST">
      <label for="ssid">WiFi Network:</label>
      <select id="ssid" name="ssid" required>
        {{options}}
      </select>
      <label for="password">Password:</label>
      <input type="password" id="password" name="password" required>
      <button type="submit">Connect</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

CxOta Ota1;
#endif /* ESP_CONSOLE_NOWIFI */

void CxESPConsoleExt::begin() {

   Led1.on();
   
#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient && !isConnected()) startWiFi();
#endif

   // set the name for this console
   setConsoleName("Ext");

#ifndef ESP_CONSOLE_NOWIFI
   if (!__bIsWiFiClient) {
      info(F("start OTA service"));
      String strPw;
      readOtaPassword(strPw);
      Ota1.onStart([](){
         CxESPConsoleExt* consoleInstance = static_cast<CxESPConsoleExt*>(CxESPConsole::getInstance());
         if(consoleInstance) {
            consoleInstance->info(F("OTA start..."));
            consoleInstance->Led1.blinkFlash();
         }
      });
      
      Ota1.onEnd([](){
         if (CxESPConsole::getInstance()) {
            CxESPConsole::getInstance()->info(F("OTA end"));
            CxESPConsole::getInstance()->reboot();
         }
      });
      
      Ota1.onProgress([](unsigned int progress, unsigned int total){
         CxESPConsoleExt* consoleInstance = static_cast<CxESPConsoleExt*>(CxESPConsole::getInstance());
         int8_t p = (int8_t)round(progress * 100 / total);
         if (consoleInstance) consoleInstance->ledAction();
         static int8_t last = 0;
         if ((p % 10)==0 && p != last) {
            if (consoleInstance) consoleInstance->info(F("OTA Progress %u"), p);
            last = p;
         }
      });
      
      Ota1.onError([](ota_error_t error){
         String strErr;
#ifdef ARDUINO
         if (error == OTA_AUTH_ERROR) {strErr = F("authorisation failed");}
         else if (error == OTA_BEGIN_ERROR) {strErr = F("begin failed");}
         else if (error == OTA_CONNECT_ERROR) {strErr = F("connect failed");}
         else if (error == OTA_RECEIVE_ERROR) {strErr = F("receive failed");}
         else if (error == OTA_END_ERROR) {strErr = F("end failed");}
#endif
         if (CxESPConsole::getInstance()) CxESPConsole::getInstance()->error(F("OTA error: %s [%d]"), strErr.c_str(), error);
      });

      Ota1.begin(getHostName(), strPw.c_str());
   }
#endif
   
   // call the begin() from base class(es) first
   CxESPConsole::begin();
   
   Led1.off();
   if (isConnected()) {
      Led1.flashOk();
   } else {
      Led1.blinkError();
   }
}

void CxESPConsoleExt::loop() {
   CxESPConsole::loop();
   if (!__isWiFiClient()) {
#ifndef ESP_CONSOLE_NOWIFI
      Ota1.loop();
#ifdef ARDUINO
      dnsServer.processNextRequest();
      webServer.handleClient();
#endif
#endif
      ledAction();
   }
}

bool CxESPConsoleExt::__processCommand(const char *szCmd, bool bQuiet) {
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
      println(F("Ext commands:" ESC_TEXT_BRIGHT_WHITE "     hw, sw, net, esp, flash, net, set, eeprom, wifi, gpio, led" ESC_ATTR_RESET));
   } else if (cmd == "hw") {
      printHW();
   } else if (cmd == "sw") {
      printSW();
   } else if (cmd == "esp") {
      printESP();
   } else if (cmd == "flash") {
      printFlashMap();
   } else if (cmd == "net") {
      printNetworkInfo();
   } else if (cmd == "set") {
      ///
      /// known env variables:
      /// - ntp <server>
      /// - tz <timezone>
      ///
      
      String strVar = TKTOCHAR(tkCmd, 1);
      if (strVar == "ntp") {
         setNtpServer(TKTOCHAR(tkCmd, 2));
      } else if (strVar == "tz") {
         setTimeZone(TKTOCHAR(tkCmd, 2));
      } else {
         println(F("set environment variable."));
         println(F("usage: set <env> <server>"));
         println(F("known env variables:\n ntp <server>\n tz <timezone>"));
         println(F("example: set ntp pool.ntp.org"));
         println(F("example: set tz CET-1CEST,M3.5.0,M10.5.0/3"));         
      }
   } else if (cmd == "eeprom") {
      if (a) {
         printEEProm(TKTOINT(tkCmd, 1, 0), TKTOINT(tkCmd, 2, 128));
      } else {
         println(F("show eeprom content."));
         println(F("usage: eeprom [<start address>] [<length>]"));
      }
   } else if (cmd == "wifi") {
      String strCmd = TKTOCHAR(tkCmd, 1);
      if (strCmd == "ssid") {
         if (b) {
            writeSSID(TKTOCHAR(tkCmd, 2));
         } else {
            char buf[20];
            ::readSSID(buf, sizeof(buf));
            print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET)); print(buf); println();
         }
      } else if (strCmd == "password") {
         if (b) {
            ::writePassword(TKTOCHAR(tkCmd, 2));
         } else {
            char buf[25];
            ::readPassword(buf, sizeof(buf));
            print(F(ESC_ATTR_BOLD "Password: " ESC_ATTR_RESET)); print(buf); println();
         }
      } else if (strCmd == "hostname") {
         if (b) {
            setHostName(TKTOCHAR(tkCmd, 2));
            ::writeHostName(TKTOCHAR(tkCmd, 2));
         } else {
            char buf[80];
            ::readHostName(buf, sizeof(buf));
            print(F(ESC_ATTR_BOLD "Hostname: " ESC_ATTR_RESET)); printHostName(); println();
         }
      } else if (strCmd == "connect") {
         startWiFi(TKTOCHAR(tkCmd, 2), TKTOCHAR(tkCmd, 3));
      } else if (strCmd == "disconnect") {
         stopWiFi();
      } else if (strCmd == "status") {
         printNetworkInfo();
      } else if (strCmd == "scan") {
         scanWiFi(*__ioStream);
      } else if (strCmd == "otapw") {
         if (b) {
            ::writeOtaPassword(TKTOCHAR(tkCmd, 2));
         } else {
            char buf[25];
            ::readOtaPassword(buf, sizeof(buf));
            print(F(ESC_ATTR_BOLD "Password: " ESC_ATTR_RESET)); print(buf); println();
         }
      } else if (strCmd == "ap") {
         if (__bIsWiFiClient) println(F("switching to AP mode. Note: this disconnects this console!"));
         delay(500);
         _beginAP();
      } else {
         printNetworkInfo();
         println(F("wifi commands:"));
         println(F("  ssid [<ssid>]"));
         println(F("  password [<password>]"));
         println(F("  hostname [<hostname>]"));
         println(F("  connect [<ssid> <password>]"));
         println(F("  disconnect"));
         println(F("  status"));
         println(F("  scan"));
         println(F("  otapw [<password>]"));
         println(F("  ap"));
      }
   } else if (cmd == "gpio") {
      String strCmd = TKTOCHAR(tkCmd, 1);
      uint8_t nPin = TKTOINT(tkCmd, 2, INVALID_PIN);
      int16_t nValue = TKTOINT(tkCmd, 3, -1);
      String strValue = TKTOCHAR(tkCmd, 3);

      if (strCmd == "state") {
         _gpioTracker.printAllStates(*__ioStream);
      } else if (strCmd == "set") {
         if (CxGPIO::isValidPin(nPin)) {
            CxGPIO gpio(nPin);
            if (nValue < 0) { // setting the pin mode
               if (strValue == "in") {
                  gpio.setPinMode(INPUT);
               } else if (strValue == "out") {
                  gpio.setPinMode(OUTPUT);
               } else if (strValue == "pwm") {
                  // todo
                  println(F("feature is not yet implemented!"));
               } else if (strValue == "inverted") {
                  gpio.setInverted(true);
               } else if (strValue == "non-inverted") {
                  gpio.setInverted(false);
               } else {
                  printf(F("invalid pin mode!"));
               }
            } else if (nValue < 1024) {
               if (nValue > HIGH && gpio.isAnalog()) {
                  println("write analog");
                  gpio.writeAnalog(nValue);
               } else {
                  println("write digital");
                  gpio.writePin(nValue);
               }
            } else {
               printf(F("invalid value!"));
            }
         } else {
            println("invalid");
            CxGPIO::printInvalidReason(*__ioStream, nPin);
         }
      } else if (strCmd == "get") {
         if (CxGPIO::isValidPin(nPin)) {
            CxGPIO gpio(nPin);
            if (gpio.isSet()) {
               gpio.printState(*__ioStream);
            }
         } else {
            CxGPIO::printInvalidReason(*__ioStream, nPin);
         }
      } else {
         _gpioTracker.printAllStates(*__ioStream);
         println(F("gpio commands:"));
         println(F("  state [<pin>]"));
         println(F("  set <pin> <mode> (in, out, pwm, inverted, non-inverted"));
         println(F("  set <pin> 0...1023 (set pin state to value)"));
         println(F("  get <pin>"));
      }
   } else if (cmd == "led") {
      String strCmd = TKTOCHAR(tkCmd, 1);
      if (strCmd == "on") {
         Led1.on();
      } else if (strCmd == "off") {
         Led1.off();
      } else if (strCmd == "blink") {
         String strPattern = TKTOCHAR(tkCmd, 2);
         if (strPattern == "ok") {
            Led1.blinkOk();
         } else if (strPattern == "error") {
            Led1.blinkError();
         } else if (strPattern == "busy") {
            Led1.blinkBusy();
         } else if (strPattern == "flash") {
            Led1.blinkFlash();
         } else if (strPattern == "data") {
            Led1.blinkData();
         } else if (strPattern == "wait") {
            Led1.blinkWait();
         } else if (strPattern == "connect") {
            Led1.blinkConnect();
         }  else {
            Led1.setBlink(TKTOINT(tkCmd, 2, 1000), TKTOINT(tkCmd, 3, 128));
         }
      } else if (strCmd == "flash") {
         String strPattern = TKTOCHAR(tkCmd, 2);
         if (strPattern == "ok") {
            Led1.flashOk();
         } else if (strPattern == "error") {
            Led1.flashError();
         } else if (strPattern == "busy") {
            Led1.flashBusy();
         } else if (strPattern == "flash") {
            Led1.flashFlash();
         } else if (strPattern == "data") {
            Led1.flashData();
         } else if (strPattern == "wait") {
            Led1.flashWait();
         } else if (strPattern == "connect") {
            Led1.flashConnect();
         } else {
            Led1.setFlash(TKTOINT(tkCmd, 2, 250), TKTOINT(tkCmd, 3, 128), TKTOINT(tkCmd, 4, 1));
         }
      } else if (strCmd == "invert") {
         Led1.setInverted(true);
      } else if (strCmd == "set") {
         uint8_t pin = TKTOINT(tkCmd, 2, INVALID_PIN);
         bool bInverted = TKTOINT(tkCmd, 2, INVALID_PIN);
         if (CxGPIO::isValidPin(pin)) {
            Led1.setPin(pin);
            Led1.setInverted(bInverted);
         } else {
            CxGPIO::printInvalidReason(*__ioStream, pin);
         }
      } else {
         printf(F("LED on pin %02d%s\n"), Led1.getPin(), Led1.isInverted() ? ",inverted":"");
         println(F("led commands:"));
         println(F("  on|off"));
         println(F("  blink [period] [duty]"));
         println(F("  blink [pattern] (ok, error...)"));
         println(F("  flash [period] [duty] [number]"));
         println(F("  set <pin> [0|1] (1: inverted)"));
      }
   } else {
      // command not handled here, proceed into the base class(es)
      return CxESPConsole::__processCommand(szCmd, bQuiet);
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

void CxESPConsoleExt::printNetworkInfo() {
#ifndef ESP_CONSOLE_NOWIFI
   print(F(ESC_ATTR_BOLD "Mode: " ESC_ATTR_RESET)); printMode();println();
   print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET)); printSSID(); printf(F(" (%s)"), isConnected()? ESC_TEXT_BRIGHT_GREEN "connected" ESC_ATTR_RESET : ESC_TEXT_BRIGHT_RED "not connected" ESC_ATTR_RESET);println();
   print(F(ESC_ATTR_BOLD "Host: " ESC_ATTR_RESET)); printHostName(); println();
   print(F(ESC_ATTR_BOLD "IP:   " ESC_ATTR_RESET)); printIp(); println();
#ifdef ARDUINO
   printf(F(ESC_ATTR_BOLD "GW:   " ESC_ATTR_RESET "%s"), WiFi.gatewayIP().toString().c_str());println();
   printf(F(ESC_ATTR_BOLD "DNS:  " ESC_ATTR_RESET "%s" ESC_ATTR_BOLD " 2nd: " ESC_ATTR_RESET "%s"), WiFi.dnsIP().toString().c_str(), WiFi.dnsIP(1).toString().c_str());println();
   printf(F(ESC_ATTR_BOLD "NTP:  " ESC_ATTR_RESET "%s"), getNtpServer());
   printf(F(ESC_ATTR_BOLD " TZ: " ESC_ATTR_RESET "%s"), getTimeZone());println();
#endif
#endif
}

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
   print(F("time to boot: ")); printTimeToBoot(*__ioStream); println();
   printf(F("free heap:    %5d Bytes\n"), ESP.getFreeHeap());
   printf(F("\n"));
#endif
}

void CxESPConsoleExt::printFlashMap() {
#ifdef ARDUINO
   printf(F("-FLASHMAP---------------\n"));
#ifdef ESP32
   printf(F("Size:         %d kBytes (0x%X)\n"), ESP.getFlashChipSize()/1024, ESP.getFlashChipSize());
#else
   printf(F("Size:         %d kBytes (0x%X)\n"), ESP.getFlashChipRealSize()/1024, ESP.getFlashChipRealSize());
#endif
   printf(F("\n"));
#ifdef ESP32
   printf(F("ESP32 Partition table:\n\n"));
   printf(F("| Type | Sub |  Offset  |   Size   |       Label      |\n"));
   printf(F("| ---- | --- | -------- | -------- | ---------------- |\n"));
   esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
   if (pi != NULL) {
      do {
         const esp_partition_t* p = esp_partition_get(pi);
         printf(F("|  %02x  | %02x  | 0x%06X | 0x%06X | %-16s |\n"),
                  p->type, p->subtype, p->address, p->size, p->label);
      } while (pi = (esp_partition_next(pi)));
   }
#else
   printf(F("Sketch start: %X\n"), getSketchStart());
   printf(F("Sketch end:   %X (%d kBytes)\n"), getSketchStart() + ESP.getSketchSize() - 0x1, ESP.getSketchSize()/1024);
   printf(F("Free start:   %X\n"), getFreeStart());
   printf(F("Free end:     %X (free: %d kBytes)\n"), getFreeEnd(), getFreeSize()/1024);
   printf(F("OTA start:    %X (with current sketch size)\n"), getOTAStart());
   printf(F("OTA end:      %X (%d kBytes)\n"), getOTAEnd(), ESP.getSketchSize()/1024);
   if (getFlashFSStart() < getWIFIEnd()) {
      printf(F("FLASHFS start: %X\n"), getFlashFSStart());
      printf(F("FLASHFS end:   %X (%d kBytes)\n"), getFlashFSEnd() - 0x1, (getFlashFSEnd() - getFlashFSStart())/1024);
   }
   printf(F("EPPROM start: %X\n"), getEPROMStart());
   printf(F("EPPROM end:   %X (%d kBytes)\n"), getEPROMEEnd() - 0x1, (getEPROMEEnd() - getEPROMStart())/1024);
   printf(F("RFCAL start:  %X\n"), getRFCALStart());
   printf(F("RFCAL end:    %X (%d kBytes)\n"), getRFCALEnd() - 0x1, (getRFCALEnd() - getRFCALStart())/1024);
   printf(F("WIFI start:   %X\n"), getWIFIStart());
   printf(F("WIFI end:     %X (%d kBytes)\n"), getWIFIEnd() - 0x1, (getWIFIEnd() - getWIFIStart())/1024);
   if (getFlashFSStart() >= getWIFIEnd()) {
      printf(F("FS start:     %X"), getFlashFSStart()); println();
      printf(F("FS end:       %X (%d kBytes)"), getFlashFSEnd() - 0x1, (getFlashFSEnd() - getFlashFSStart())/1024);
   }

#endif
   printf(F("\n"));
   printf(F("------------------------\n"));
#endif
}

void CxESPConsoleExt::ledAction() {
   Led1.action();
}

#ifndef ESP_CONSOLE_NOWIFI

void CxESPConsoleExt::printEEProm(uint32_t nStartAddr, uint32_t nLength) {
   printEEPROM(*__ioStream, nStartAddr, nLength);
}
   
void CxESPConsoleExt::readSSID(String& strSSID) {
   char buf[20];
   ::readSSID(buf, sizeof(buf));
   strSSID = buf;
}

void CxESPConsoleExt::readPassword(String& strPassword) {
   char buf[25];
   ::readPassword(buf, sizeof(buf));
   strPassword = buf;
}

void CxESPConsoleExt::readHostName(String& strHostName) {
   char buf[80];
   ::readHostName(buf, sizeof(buf));
   strHostName = buf;
}

void CxESPConsoleExt::readOtaPassword(String& strPassword) {
   char buf[25];
   ::readOtaPassword(buf, sizeof(buf));
   strPassword = buf;
}

void CxESPConsoleExt::startWiFi(const char* ssid, const char* pw) {

   _stopAP();
   
   if (isConnected()) {
      stopWiFi();
   }
   
   //
   // Set the ssid, password and hostname from the console settings or from the arguments.
   // If set by the arguments, it will replace settings stored in the eprom.
   //
   // All can be set in the console with the commands
   //   wifi ssid <ssid>
   //   wifi password <password>
   //   wifi hostname <hostname>
   // These settings will be stored in the EEPROM.
   //
   
   String strSSID;
   String strPassword;
   String strHostName;
   
   if (ssid) writeSSID(ssid);
   readSSID(strSSID);
   if (pw) writePassword(pw);
   readPassword(strPassword);
   readHostName(strHostName);
   
#ifdef ARDUINO
   WiFi.persistent(false);
   WiFi.mode(WIFI_STA);
   WiFi.begin(strSSID.c_str(), strPassword.c_str());
   WiFi.setAutoReconnect(true);
   WiFi.hostname(strHostName.c_str());
   
   println();
   printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s" ESC_ATTR_RESET), strSSID.c_str());
   print(F(ESC_ATTR_BLINK "..." ESC_ATTR_RESET));
   
   Led1.blinkConnect();
   
   // try to connect to the network for max. 10 seconds
   CxTimer10s timerTO; // set timeout
   
   while (WiFi.status() != WL_CONNECTED && !timerTO.isDue()) {
      Led1.action();
      delay(1);
   }
   
   print(ESC_CLEAR_LINE "\r");
   printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s..." ESC_ATTR_RESET), strSSID.c_str());

   Led1.off();
   
   if (WiFi.status() != WL_CONNECTED) {
      println(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "not connected!" ESC_ATTR_RESET));
      error("WiFi not connected.");
      Led1.blinkError();
   } else {
      println(F(ESC_TEXT_BRIGHT_GREEN "connected!" ESC_ATTR_RESET));
      info("WiFi connected.");
      __updateTime();
      Led1.flashOk();
   }

#endif
}

void CxESPConsoleExt::stopWiFi() {
   info(F("WiFi disconnect and switch off."));
   printf(F("WiFi disconnect and switch off."));
#ifdef ARDUINO
   WiFi.disconnect();
   WiFi.softAPdisconnect();
   WiFi.mode(WIFI_OFF);
   WiFi.forceSleepBegin();
#endif
}

void CxESPConsoleExt::_handleRoot() {
#ifdef ARDUINO
   String htmlPage = htmlPageTemplate;
   
   // Scan for available Wi-Fi networks
   int n = WiFi.scanNetworks();
   String options = "";
   
   if (n == 0) {
      options = "<option value=\"\">No networks found</option>";
   } else {
      for (int i = 0; i < n; ++i) {
         // Get network name (SSID) and signal strength
         String ssid = WiFi.SSID(i);
         int rssi = WiFi.RSSI(i);
         options += "<option value=\"" + ssid + "\">" + ssid + " (Signal: " + String(rssi) + " dBm)</option>";
      }
   }
   
   // Replace placeholder with actual network options
   htmlPage.replace("{{options}}", options);
   
   webServer.send(200, "text/html", htmlPage);
#endif
}

void CxESPConsoleExt::_handleConnect() {
#ifdef ARDUINO
   CxESPConsoleExt* pConsole = static_cast<CxESPConsoleExt*>(CxESPConsole::getInstance());
   
   if (pConsole && webServer.hasArg("ssid") && webServer.hasArg("password")) {
      String ssid = webServer.arg("ssid");
      String password = webServer.arg("password");
      
      webServer.send(200, "text/plain", "Attempting to connect to WiFi...");
      pConsole->info(F("SSID: %s, Password: %s"), ssid.c_str(), password.c_str());
      
      // Attempt WiFi connection
      WiFi.begin(ssid.c_str(), password.c_str());
      
      // Wait a bit to connect
      CxTimer10s timerTO; // set timeout

      while (WiFi.status() != WL_CONNECTED && timerTO.isDue()) {
         delay(100);
      }
      
      if (WiFi.status() == WL_CONNECTED) {
         pConsole->info("Connected successfully!");
         webServer.send(200, "text/plain", "Connected to WiFi!");
         
         // switch to STA mode, saves credentials and stop web and dns server.
         pConsole->startWiFi(ssid.c_str(), password.c_str());
      } else {
         if (pConsole) pConsole->error("Connection failed.");
         webServer.send(200, "text/plain", "Failed to connect. Check credentials.");
      }
   } else {
      webServer.send(400, "text/plain", "Missing SSID or Password");
   }

#endif
}

void CxESPConsoleExt::_beginAP() {
   stopWiFi();
   
   Led1.blinkWait();

#ifdef ARDUINO
   // Start Access Point
   WiFi.softAP(getHostName(), "12345678");
   
   // Start DNS Server
   dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

   
   // Define routes
   webServer.on("/", _handleRoot);
   webServer.on("/connect", HTTP_POST, _handleConnect);
   webServer.onNotFound([]() {
      webServer.sendHeader("Location", "/", true); // Redirect to root
      webServer.send(302, "text/plain", "Redirecting to Captive Portal");
   });
   
   // Start the web server
   webServer.begin();
#endif
   info(F("ESP started in AP mode"));
   printf(F("ESP started in AP mode. SSID: %s, PW:%s\n"), getHostName(), "12345678");
}

void CxESPConsoleExt::_stopAP() {
   Led1.off();
   
#if defined(ESP32)
   webServer.stop();
#elif defined(ESP8266)
   webServer.close();
   dnsServer.stop();
#endif
}

#endif /* ESP_CONSOLE_NOWIFI */

