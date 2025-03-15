/**
 * @file CxCapabilityExt.hpp
 * @brief This file defines the CxCapabilityExt class, which extends the CxCapability class.
 *
 * The CxCapabilityExt class manages various capabilities and functionalities for an ESP-based project,
 * including WiFi management, OTA updates, GPIO control, and sensor management.
 *
 * @date Created by ocfu on 09.01.25.
 *
 */


#ifndef CxCapabilityExt_hpp
#define CxCapabilityExt_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityBasic.hpp"

#include "../tools/CxGpioTracker.hpp"
#include "../tools/CxLed.hpp"
#include "esphw.h"
#include "../tools/CxSensorManager.hpp"

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
CxLed Led1(LED_BUILTIN);

bool g_bOTAinProgress = false;

#endif /* ESP_CONSOLE_NOWIFI */

class CxCapabilityExt : public CxCapability {
   CxTimer10s _nTimer10s;
   
   CxESPConsoleMaster& console = CxESPConsoleMaster::getInstance();
   CxGPIOTracker& _gpioTracker = CxGPIOTracker::getInstance();
   
   CxSensorManager& _sensorManager = CxSensorManager::getInstance();

public:

   explicit CxCapabilityExt() : CxCapability("ext", getCmds()) {}
   static constexpr const char* getName() { return "ext"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "hw", "sw", "esp", "flash", "set", "eeprom", "wifi", "gpio", "led", "ping", "sensor" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityExt>();
   }
   
   ~CxCapabilityExt() {
      Ota1.end();
      stopWiFi();
   }
   
   void setup() override {
      CxCapability::setup();

      g_Heap.update();
      
      __bLocked = false;
      
      console.info(F("====  Cap: %s  ===="), getName());
      
      if (!isConnected()) {
         println();
         startWiFi();
      }
      
      Led1.off();
      if (isConnected()) {
         Led1.flashOk();
      } else {
         Led1.blinkError();
      }
      
      // ota
      console.info(F("start OTA service"));
      char szOtaPassword[25];
      ::readOtaPassword(szOtaPassword, sizeof(szOtaPassword));
      
      Ota1.onStart([](){
         CxESPConsoleMaster& con = CxESPConsoleMaster::getInstance();
         con.info(F("OTA start..."));
         Led1.blinkFlash();
         g_bOTAinProgress = true;
      });
      
      Ota1.onEnd([](){
         CxESPConsoleMaster& con = CxESPConsoleMaster::getInstance();
         con.info(F("OTA end"));
         if (g_bOTAinProgress) {
            con.processCmd("reboot -f");
         }
         g_bOTAinProgress = false;
      });
      
      Ota1.onProgress([](unsigned int progress, unsigned int total){
         CxESPConsoleMaster& con = CxESPConsoleMaster::getInstance();
         int8_t p = (int8_t)round(progress * 100 / total);
         Led1.action();
         static int8_t last = 0;
         if ((p % 10)==0 && p != last) {
            con.info(F("OTA Progress %u"), p);
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
         CxESPConsoleMaster& con = CxESPConsoleMaster::getInstance();

         con.error(F("OTA error: %s [%d]"), strErr.c_str(), error);
      });
      
      Ota1.begin(console.getHostName(), szOtaPassword);
   }
   
   void loop() override {
#ifndef ESP_CONSOLE_NOWIFI
      Ota1.loop();
#ifdef ARDUINO
      dnsServer.processNextRequest();
      webServer.handleClient();
#endif
#endif

      ledAction();
      
      if (_nTimer10s.isDue()) {
         g_Heap.update();
         _sensorManager.update();
      }
   }
   
    bool execute(const char *szCmd) override {
       
      // validate the call
      if (!szCmd) return false;
      
      // get the command and arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
       String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      // expect sz parameter, invalid is nullptr
      const char* a = TKTOCHAR(tkArgs, 1);
      const char* b = TKTOCHAR(tkArgs, 2);
      
       if (cmd == "?") {
          printCommands();
       } else if (cmd == "hw") {
         printHW();
      } else if (cmd == "sw") {
         printSW();
      } else if (cmd == "esp") {
         printESP();
      } else if (cmd == "flash") {
         printFlashMap();
      } else if (cmd == "set") {
         ///
         /// known env variables:
         /// - ntp <server>
         /// - tz <timezone>
         ///
         
         String strVar = TKTOCHAR(tkArgs, 1);
         if (strVar == "ntp") {
            console.setNtpServer(TKTOCHAR(tkArgs, 2));
         } else if (strVar == "tz") {
            console.setTimeZone(TKTOCHAR(tkArgs, 2));
         } else {
            println(F("set environment variable."));
            println(F("usage: set <env> <server>"));
            println(F("known env variables:\n ntp <server>\n tz <timezone>"));
            println(F("example: set ntp pool.ntp.org"));
            println(F("example: set tz CET-1CEST,M3.5.0,M10.5.0/3"));
         }
      } else if (cmd == "eeprom") {
         if (a) {
            ::printEEPROM(getIoStream(), TKTOINT(tkArgs, 1, 0), TKTOINT(tkArgs, 2, 128));
         } else {
            println(F("show eeprom content."));
            println(F("usage: eeprom [<start address>] [<length>]"));
         }
      } else if (cmd == "wifi") {
         String strCmd = TKTOCHAR(tkArgs, 1);
         if (strCmd == "ssid") {
            if (b) {
               ::writeSSID(TKTOCHAR(tkArgs, 2));
            } else {
               char buf[20];
               ::readSSID(buf, sizeof(buf));
               print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET)); print(buf); println();
            }
         } else if (strCmd == "password") {
            if (b) {
               ::writePassword(TKTOCHAR(tkArgs, 2));
            } else {
               char buf[25];
               ::readPassword(buf, sizeof(buf));
               print(F(ESC_ATTR_BOLD "Password: " ESC_ATTR_RESET)); print(buf); println();
            }
         } else if (strCmd == "hostname") {
            if (b) {
               console.setHostName(TKTOCHAR(tkArgs, 2));
               ::writeHostName(TKTOCHAR(tkArgs, 2));
            } else {
               char buf[80];
               ::readHostName(buf, sizeof(buf));
               print(F(ESC_ATTR_BOLD "Hostname: " ESC_ATTR_RESET)); print(buf); println();
            }
         } else if (strCmd == "connect") {
            startWiFi(TKTOCHAR(tkArgs, 2), TKTOCHAR(tkArgs, 3));
         } else if (strCmd == "disconnect") {
            stopWiFi();
         } else if (strCmd == "status") {
            console.processCmd("net");
         } else if (strCmd == "scan") {
            ::scanWiFi(getIoStream());
         } else if (strCmd == "otapw") {
            if (b) {
               ::writeOtaPassword(TKTOCHAR(tkArgs, 2));
            } else {
               char buf[25];
               ::readOtaPassword(buf, sizeof(buf));
               print(F(ESC_ATTR_BOLD "Password: " ESC_ATTR_RESET)); print(buf); println();
            }
         } else if (strCmd == "ap") {
            if (console.isWiFiClient()) println(F("switching to AP mode. Note: this disconnects this console!"));
            delay(500);
            _beginAP();
         } else {
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
            println(F("  sensor"));
         }
      } else if (cmd == "ping") {
         if (!a && !b) {
            println(F("usage: ping <host> <port>"));
         } else {
            if (isHostAvailble(TKTOCHAR(tkArgs, 1), TKTOINT(tkArgs, 2, 0))) {
               println(F("ok"));
            } else {
               println(F("host not available on this port!"));
            };
         }
      } else if (cmd == "gpio") {
         String strCmd = TKTOCHAR(tkArgs, 1);
         uint8_t nPin = TKTOINT(tkArgs, 2, INVALID_PIN);
         int16_t nValue = TKTOINT(tkArgs, 3, -1);
         String strValue = TKTOCHAR(tkArgs, 3);
         
         if (strCmd == "state") {
            _gpioTracker.printAllStates(getIoStream());
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
               CxGPIO::printInvalidReason(getIoStream(), nPin);
            }
         } else if (strCmd == "get") {
            if (CxGPIO::isValidPin(nPin)) {
               CxGPIO gpio(nPin);
               if (gpio.isSet()) {
                  gpio.printState(getIoStream());
               }
            } else {
               CxGPIO::printInvalidReason(getIoStream(), nPin);
            }
         } else {
            _gpioTracker.printAllStates(getIoStream());
            println(F("gpio commands:"));
            println(F("  state [<pin>]"));
            println(F("  set <pin> <mode> (in, out, pwm, inverted, non-inverted"));
            println(F("  set <pin> 0...1023 (set pin state to value)"));
            println(F("  get <pin>"));
         }
      } else if (cmd == "led") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         if (strSubCmd == "on") {
            Led1.on();
         } else if (strSubCmd == "off") {
            Led1.off();
         } else if (strSubCmd == "blink") {
            String strPattern = TKTOCHAR(tkArgs, 2);
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
               Led1.setBlink(TKTOINT(tkArgs, 2, 1000), TKTOINT(tkArgs, 3, 128));
            }
         } else if (strSubCmd == "flash") {
            String strPattern = TKTOCHAR(tkArgs, 2);
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
               Led1.setFlash(TKTOINT(tkArgs, 2, 250), TKTOINT(tkArgs, 3, 128), TKTOINT(tkArgs, 4, 1));
            }
         } else if (strSubCmd == "invert") {
            Led1.setInverted(!Led1.isInverted());
            Led1.toggle();
         } else {
            printf(F("LED on pin %02d%s\n"), Led1.getPin(), Led1.isInverted() ? ",inverted":"");
            println(F("led commands:"));
            println(F("  on|off"));
            println(F("  blink [period] [duty]"));
            println(F("  blink [pattern] (ok, error...)"));
            println(F("  flash [period] [duty] [number]"));
            println(F("  invert"));
         }
      } else if (cmd == "sensor") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         if (strSubCmd == "list") {
            _sensorManager.printList();
         } else if (strSubCmd == "get") {
            float f = _sensorManager.getSensorValueFloat(TKTOINT(tkArgs, 2, INVALID_FLOAT));
            if (!std::isnan(f)) {
               println(f);
            } else {
               println(F("invalid sensor id!"));
            }
         } else {
            println(F("sensor commands:"));
            println(F("  list"));
            println(F("  get <id>"));
         }
      } else {
         return false;
      }
       g_Stack.update();
      return true;
   }
   
   void printHW() {
      printf(F(ESC_ATTR_BOLD "    Chip Type:" ESC_ATTR_RESET " %s " ESC_ATTR_BOLD "Chip-ID: " ESC_ATTR_RESET "0x%X\n"), getChipType(), getChipId());
#ifdef ARDUINO
      printf(F(ESC_ATTR_BOLD "   Flash Size:" ESC_ATTR_RESET " %dk (real) %dk (ide)\n"), getFlashChipRealSize()/1024, getFlashChipSize()/1024);
      printf(F(ESC_ATTR_BOLD "Chip-Frequenz:" ESC_ATTR_RESET " %dMHz\n"), ESP.getCpuFreqMHz());
#endif
   }
   
   void printSW() {
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
      if (console.getAppName()[0]) printf(F(ESC_ATTR_BOLD "    Firmware:" ESC_ATTR_RESET " %s Ver.:" ESC_ATTR_RESET " %s\n"), console.getAppName(), console.getAppVer());
   }
   
   void printESP() {
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
      printf(F("Application:  %s (%s)\n"), console.getAppName(), console.getAppVer());
      printf(F("\n"));
      printf(F("-BOOT-------------------\n"));
      printf(F("reset reason: %s\n"), getResetInfo());
      print(F("time to boot: ")); console.printTimeToBoot(getIoStream()); println();
      printf(F("free heap:    %5d Bytes\n"), ESP.getFreeHeap());
      printf(F("\n"));
#endif
   }

   void printFlashMap() {
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
   
#ifndef ESP_CONSOLE_NOWIFI
   bool isConnected() {
#ifdef ARDUINO
      return (WiFi.status() == WL_CONNECTED);
#else
      return false;
#endif
   }

   bool isHostAvailble(const char* host, uint32_t port) {
#ifdef ARDUINO
      if (isConnected() && port && host) { //Check WiFi connection status
         WiFiClient client;
         if (client.connect(host, port)) {
            client.stop();
            return true;
         }
      }
#endif
      return false;
   }
 
#endif /* ESP_CONSOLE_NOWIFI */

   void ledAction() {
      Led1.action();
   }

   void startWiFi(const char* ssid = nullptr, const char* pw = nullptr) {
      
#ifndef ESP_CONSOLE_NOWIFI
      //_stopAP();
      
      if (isConnected()) {
         //stopWiFi();
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
      
      char szSSID[20];
      char szPassword[25];
      char szHostname[80];
      
      if (ssid) ::writeSSID(ssid);
      ::readSSID(szSSID, sizeof(szSSID));
            
      if (pw) ::writePassword(pw);
      ::readPassword(szPassword, sizeof(szPassword));
      
      ::readHostName(szHostname, sizeof(szHostname));
      
#ifdef ARDUINO
      WiFi.persistent(false);
      WiFi.mode(WIFI_STA);
      WiFi.begin(szSSID, szPassword);
      WiFi.setAutoReconnect(true);
      WiFi.hostname(szHostname);
      
      printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s" ESC_ATTR_RESET), szSSID);
      print(F(ESC_ATTR_BLINK "..." ESC_ATTR_RESET));
      
      Led1.blinkConnect();
      
      // try to connect to the network for max. 10 seconds
      CxTimer10s timerTO; // set timeout
      
      while (WiFi.status() != WL_CONNECTED && !timerTO.isDue()) {
         Led1.action();
         delay(1);
      }
      
      print(ESC_CLEAR_LINE "\r");
      printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s..." ESC_ATTR_RESET), szSSID);
      
      Led1.off();
      
      if (WiFi.status() != WL_CONNECTED) {
         println(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "not connected!" ESC_ATTR_RESET));
         console.error("WiFi not connected.");
         Led1.blinkError();
      } else {
         println(F(ESC_TEXT_BRIGHT_GREEN "connected!" ESC_ATTR_RESET));
         console.info("WiFi connected.");
         Led1.flashOk();
      }
      
#endif /* Arduino */
   }
   
   void stopWiFi() {
      console.info(F("WiFi disconnect and switch off."));
      println(F("WiFi disconnect and switch off."));
#ifdef ARDUINO
      WiFi.disconnect();
      WiFi.softAPdisconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
#endif
   }
   
private:
   static void _handleRoot() {
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

   static void _handleConnect() {
#ifdef ARDUINO
      
      if (webServer.hasArg("ssid") && webServer.hasArg("password")) {
         String ssid = webServer.arg("ssid");
         String password = webServer.arg("password");
         
         CxESPConsoleMaster& con = CxESPConsoleMaster::getInstance();

         
         webServer.send(200, "text/plain", "Attempting to connect to WiFi...");
         con.info(F("SSID: %s, Password: %s"), ssid.c_str(), password.c_str());
         
         // Attempt WiFi connection
         WiFi.begin(ssid.c_str(), password.c_str());
         
         // Wait a bit to connect
         CxTimer10s timerTO; // set timeout
         
         while (WiFi.status() != WL_CONNECTED && !timerTO.isDue()) {
            delay(1);
         }
         
         if (WiFi.status() == WL_CONNECTED) {
            con.info("Connected successfully!");
            webServer.send(200, "text/plain", "Connected to WiFi!");
            
            // switch to STA mode, saves credentials and stop web and dns server.
            String cmd = "wifi connect ";
            cmd += ssid + " " + password;
            con.processCmd(cmd.c_str());
         } else {
            con.error("Connection failed.");
            webServer.send(200, "text/plain", "Failed to connect. Check credentials.");
         }
      } else {
         webServer.send(400, "text/plain", "Missing SSID or Password");
      }
      
#endif
   }
   
   void _beginAP() {
      stopWiFi();
      
      Led1.blinkWait();
      
#ifdef ARDUINO
      // Start Access Point
      WiFi.softAP(console.getHostName(), "12345678");
      
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
      console.info(F("ESP started in AP mode"));
      printf(F("ESP started in AP mode. SSID: %s, PW:%s\n"), console.getHostName(), "12345678");
      console.setAPMode(true);
   }
   
   void _stopAP() {
      Led1.off();
      
#if defined(ESP32)
      webServer.stop();
#elif defined(ESP8266)
      webServer.close();
      dnsServer.stop();
#endif
      console.setAPMode(false);
   }


#endif /* ESP_CONSOLE_NOWIFI */

   
};

#endif /* CxCapabilityExt */

