/**
 * @file CxCapabilityExt.hpp
 * @brief This file defines the CxCapabilityExt class, which extends the CxCapability class.
 *
 * The CxCapabilityExt class manages various capabilities and functionalities for an ESP-based project,
 * including WiFi management, OTA updates, GPIO control, and sensor management.
 *
 * Dependencies:
 * - CxCapability.hpp: Base class for capabilities
 * - CxESPConsole.hpp: Console management class
 * - CxCapabilityBasic.hpp: Basic capability class
 * - CxGpioTracker.hpp: GPIO tracker class
 * - CxLed.hpp: LED control class
 * - CxSensorManager.hpp: Sensor manager class
 * - CxOta.hpp: OTA update class
 * - WebServer.h: Web server library for ESP32
 * - ESP8266WebServer.h: Web server library for ESP8266
 * - DNSServer.h: DNS server library
 *
 * @date Created by ocfu on 09.01.25.
 * @copyright © 2025 ocfu
 *
 * Key Features:
 * 1. Classes and Enumerations:
 *    - CxCapabilityExt: Extends CxCapability to manage various functionalities.
 *
 * 2. CxCapabilityExt Class:
 *    - Manages WiFi, OTA updates, GPIO control, and sensor management.
 *    - Provides methods to enable/disable WiFi, set/get sensor values, and update sensor readings.
 *    - Registers and unregisters sensors with the CxSensorManager.
 *
 * Relationships:
 * - CxCapabilityExt is a subclass of CxCapability and interacts with CxSensorManager to register/unregister sensors.
 *
 * How They Work Together:
 * - CxCapabilityExt represents various capabilities with specific properties and methods.
 * - Sensors register themselves with the CxSensorManager upon creation and unregister upon destruction.
 * - The CxCapabilityExt can initialize, end, and print sensor information.
 *
 * Suggested Improvements:
 * 1. Error Handling:
 *    - Add error handling for edge cases, such as invalid sensor IDs or failed sensor updates.
 *
 * 2. Code Refactoring:
 *    - Improve code readability and maintainability by refactoring complex methods and reducing code duplication.
 *
 * 3. Documentation:
 *    - Enhance documentation with more detailed explanations of methods and their parameters.
 *
 * 4. Testing:
 *    - Implement unit tests to ensure the reliability and correctness of the sensor management functionality.
 *
 * 5. Resource Management:
 *    - Monitor and optimize resource usage, such as memory and processing time, especially for embedded systems with limited resources.
 *
 * 6. Extensibility:
 *    - Provide a more flexible mechanism for adding new sensor types and capabilities without modifying the core classes.
 */


#ifndef CxCapabilityExt_hpp
#define CxCapabilityExt_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityBasic.hpp"

#include "../tools/CxGpioTracker.hpp"
#include "../tools/CxLed.hpp"
#include "../tools/CxButton.hpp"
#include "../tools/CxRelay.hpp"
#include "../tools/CxContact.hpp"
#include "../tools/CxAnalog.hpp"
#include "esphw.h"
#include "../tools/CxSensorManager.hpp"
#include "../tools/espmath.h"

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

#if !defined(CxCapabilityFS_hpp)

// HTML page for the AP without CSS style to save some bin size
const char htmlPageTemplate[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WiFi Setup</title>
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
#else

#ifdef ARDUINO
#include <FS.h>
#ifdef ESP32
#include "LITTLEFS.h"
struct FSInfo {
   size_t totalBytes;
   size_t usedBytes;
   size_t blockSize;
   size_t pageSize;
   size_t maxOpenFiles;
   size_t maxPathLength;
};
#define Dir File
#define LittleFS LITTLEFS
#else
#include <LittleFS.h>
#endif /* ESP32*/
#endif /* ARDUINO */

#endif /* !defined(CxCapabilityFS_hpp)  */

/// global objects for OTA and LED
CxOta Ota1;
CxLed Led1(LED_BUILTIN, "led1");

bool g_bOTAinProgress = false;

#endif /* ESP_CONSOLE_NOWIFI */

/**
 * @brief CxCapabilityExt class for managing various capabilities and functionalities.
 * @details The CxCapabilityExt class extends the CxCapability class to manage capabilities such as WiFi, OTA updates, GPIO control, and sensor management.
 * It provides methods to enable/disable WiFi, set/get sensor values, and update sensor readings.
 * The class registers and unregisters sensors with the CxSensorManager.
 *
 */
class CxCapabilityExt : public CxCapability {
   /// access to the instances of the master console, GPIO tracker, sensor manager
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();
   CxGPIOTracker& _gpioTracker = CxGPIOTracker::getInstance();
   CxGPIODeviceManagerManager& _gpioDeviceManager = CxGPIODeviceManagerManager::getInstance();
   CxSensorManager& _sensorManager = CxSensorManager::getInstance();
   CxGPIOTracker& __gpioTracker = CxGPIOTracker::getInstance(); // Reference to the GPIO tracker singleton
   
   /// timer for updating stack info and sensor data
   CxTimer10s _timerUpdate;
   
   bool _bWifiConnected = false;
   
   std::map<String, String> _mapProcessJsonDataItems;
   
public:
   /// Default constructor and default capabilities methods
   explicit CxCapabilityExt() : CxCapability("ext", getCmds()) {}
   static constexpr const char* getName() { return "ext"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "hw", "sw", "esp", "flash", "set", "eeprom", "wifi", "gpio", "led", "ping", "sensor", "relay", "processdata", "smooth", "id", "app" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityExt>();
   }
   
   /// Destructor to end the capability and stop OTA and Wifi
   ~CxCapabilityExt() {
      Ota1.end();
      stopWiFi();
   }
   
   /// Setup method to initialize the capability and connect to wifi, if not connected.
   void setup() override {
      CxCapability::setup();

      g_Heap.update();
      
      __bLocked = false;
      
      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());
    
     
      /// setup OTA service
      _CONSOLE_INFO(F("start OTA service"));
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
      
      Ota1.begin(__console.getHostName(), szOtaPassword);
   }
   
   /// Loop method to update sensor data, handle OTA updates, and manage LED status and web server requests.
   void loop() override {
#ifndef ESP_CONSOLE_NOWIFI
      Ota1.loop();
#ifdef ARDUINO
      dnsServer.processNextRequest();
      webServer.handleClient();
#endif
#endif

      /// update led indications, if any
      ledAction();
      
      _sensorManager.update();

      /// check gpio events
      gpioAction();
      
      /// update sensor data and stack info
      if (_timerUpdate.isDue()) {
         g_Heap.update();
      }
      
   }
   
   /// Execute method to process the given command and return the result.
   uint8_t execute(const char *szCmd, uint8_t nClient) override {
       
      // validate the call
      if (!szCmd) return EXIT_FAILURE;
      
      // get the command and arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
       String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      // expect sz parameter, invalid is nullptr
      const char* a = TKTOCHAR(tkArgs, 1);
      const char* b = TKTOCHAR(tkArgs, 2);
      
      uint32_t nExitValue = EXIT_FAILURE; // default exit value is 1 (error)
      
       if (cmd == "?") {
          nExitValue = printCommands();
       } else if (cmd == "hw") {
          printHW();
          nExitValue = EXIT_SUCCESS;
          __console.setOutputVariable(getChipType());
       } else if (cmd == "id") {
          __console.setOutputVariable(getChipId());
          nExitValue = EXIT_SUCCESS;
       } else if (cmd == "sw") {
          printSW();
          __console.setOutputVariable(__console.getAppVer());
          nExitValue = EXIT_SUCCESS;
       } else if (cmd == "app") {
          __console.setOutputVariable(__console.getAppName());
          nExitValue = EXIT_SUCCESS;
       } else if (cmd == "esp") {
          printESP();
#ifdef ARDUINO
#ifdef ESP32
         //TODO: implement esp core version for esp32
#else
         __console.setOutputVariable(ESP.getCoreVersion().c_str());
#endif
#endif /* ARDUINO */
          nExitValue = EXIT_SUCCESS;
      } else if (cmd == "flash") {
         printFlashMap();
#ifdef ARDUINO
         __console.setOutputVariable(ESP.getFlashChipSize()/1024);
#endif
         nExitValue = EXIT_SUCCESS;
      } else if (cmd == "set") {
         String strVar = TKTOCHAR(tkArgs, 1);
         uint8_t prec = 0;
         String strOp1 = TKTOCHAR(tkArgs, 2);
         bool bIsExpr = false;
         
         String strValue;
         strValue.reserve(128);  // 128 is an estimate. The worst case is a concat of two strings, while both could be a variable with a max lenght of ?

         if (strOp1 != "=") {
            strValue = TKTOCHARAFTER(tkArgs, 2);
         } else {
            strValue = TKTOCHARAFTER(tkArgs, 3);
            bIsExpr = true;
         }
         int32_t nPrecIndex = strVar.indexOf("/");
         if ( nPrecIndex > 0) {
            prec = strVar.substring(nPrecIndex+1).toInt();
            strVar = strVar.substring(0, nPrecIndex);
         }
         
         if (strVar == "TZ") {
            __console.setTimeZone(strValue.c_str());
            __console.addVariable(strVar.c_str(), strValue.c_str());
            nExitValue = EXIT_SUCCESS;
         } else if (strVar == "BUF") {
            uint32_t nBufLen = (uint32_t)strValue.toInt();
            if (nBufLen >= 64) {
               __console.setCmdBufferLen(nBufLen);
               __console.addVariable(strVar.c_str(), String(__console.getCmdBufferLen()).c_str());
               nExitValue = EXIT_SUCCESS;
            }
         } else if (strVar.length() > 0) {
            bool bValid = true;
            
            if (bIsExpr) {
               ExprParser parser;
               float fValue = 0.0F;

               fValue = parser.eval(strValue.c_str(), bValid);
               
               if (bValid) {
                  strValue = String(fValue, prec);
               } else {
                  strValue = "nan";
               }
            }
            
            strValue.trim();
            
            if (strValue.length() > 0) {
               __console.addVariable(strVar.c_str(), strValue.c_str());
            } else {
               __console.removeVariable(strVar.c_str());
            }
            if (strVar != "?") {
               nExitValue = bValid ? 0 : 1;
            }
         } else {
            /// print all variables
            __console.printVariables(getIoStream());
            nExitValue = EXIT_SUCCESS;
         }
      } else if (cmd == "eeprom") {
         if (a) {
            ::printEEPROM(getIoStream(), TKTOINT(tkArgs, 1, 0), TKTOINT(tkArgs, 2, 128));
            nExitValue = EXIT_SUCCESS;
         } else {
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("show eeprom content."));
               println(F("usage: eeprom [<start address>] [<length>]"));
#endif
            }
         }
      } else if (cmd == "wifi") {
         String strCmd = TKTOCHAR(tkArgs, 1);
         nExitValue = EXIT_SUCCESS;
         if (strCmd == "ssid") {
            if (b) {
               ::writeSSID(TKTOCHAR(tkArgs, 2));
            } else {
               char buf[20];
               ::readSSID(buf, sizeof(buf));
               print(F(ESC_ATTR_BOLD "SSID: " ESC_ATTR_RESET)); print(buf); println();
               __console.setOutputVariable(buf);
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
               __console.setHostName(TKTOCHAR(tkArgs, 2));
               ::writeHostName(TKTOCHAR(tkArgs, 2));
            } else {
               char buf[80];
               ::readHostName(buf, sizeof(buf));
               print(F(ESC_ATTR_BOLD "Hostname: " ESC_ATTR_RESET)); print(buf); println();
               __console.setOutputVariable(buf);
            }
         } else if (strCmd == "connect") {
            startWiFi(TKTOCHAR(tkArgs, 2), TKTOCHAR(tkArgs, 3));
         } else if (strCmd == "disconnect") {
            stopWiFi();
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
            if (__console.isWiFiClient()) println(F("switching to AP mode. Note: this disconnects this console!"));
            delay(500);
            _beginAP();
         } else if (strCmd == "check") {
            bool bStatus = checkWifi();
            if (!b) {
               print(F("WiFi is "));
               if (bStatus) {
                  println(F("connected"));
               } else {
                  println(F("not connected"));
                  nExitValue = EXIT_FAILURE;
               }
            }               
         } else if (strCmd == "rssi") {
#ifdef ARDUINO
            print(WiFi.RSSI()); println(F("dBm"));
            __console.setOutputVariable(WiFi.RSSI());
#endif
         }
         else {
            nExitValue = EXIT_FAILURE;
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
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
               println(F("  check [-q]"));
#endif
            }
         }
      } else if (cmd == "ping") {
         if (!a && !b) {
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
               println(F("usage: ping <host> <port>"));
            }
         } else {
            if (isHostAvailble(TKTOCHAR(tkArgs, 1), TKTOINT(tkArgs, 2, 0))) {
               println(F("ok"));
               nExitValue = EXIT_SUCCESS;
            } else {
               println(F("host not available on this port!"));
            };
         }
      } else if (cmd == "gpio") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         uint8_t nPin = TKTOINT(tkArgs, 2, INVALID_PIN);
         int16_t nValue = TKTOINT(tkArgs, 3, -1);
         String strValue = TKTOCHAR(tkArgs, 3);
         String strEnv = ".gpio";
         
         if (strSubCmd == "state") {
            CxTablePrinter table(getIoStream());
#ifndef MINIMAL_COMMAND_SET
            table.printHeader({F("Pin"), F("Mode"), F("inv"), F("State"), F("PWM"), F("Value")}, {3, 10, 3, 5, 8, 6});
#else
            table.printHeader({F("Pin"), F("Mode"), F("inv"), F("State")}, {3, 10, 3, 5});
#endif
            
            for (const auto& pin : _gpioTracker.getPins()) {
               if (nPin != INVALID_PIN && nPin != pin) continue; // skip if pin does not match

               CxGPIO gpio(pin);
               gpio.get();
               if (gpio.isAnalog()) {
                  if (nPin != INVALID_PIN) {
                     __console.setOutputVariable(gpio.getAnalogValue());
                  }
#ifndef MINIMAL_COMMAND_SET
                  table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", "n/a", "n/a", gpio.isAnalog() ? String(gpio.getAnalogValue()):""});
#else
                  table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", "n/a"});
#endif
               } else {
                  if (nPin != INVALID_PIN) {
                     __console.setOutputVariable(gpio.getDigitalState() ? "HIGH" : "LOW");
                  }
#ifndef MINIMAL_COMMAND_SET
                  table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", gpio.getDigitalState() ? "HIGH" : "LOW", gpio.isPWM() ? "Endabled" : "Disabled", gpio.isAnalog() ? String(gpio.getAnalogValue()):""});
#else
                  table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", gpio.getDigitalState() ? "HIGH" : "LOW"});
#endif
               }
            }
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "set") {
            if (__gpioTracker.isValidPin(nPin)) {
               CxGPIO gpio(nPin);
               nExitValue = EXIT_SUCCESS;
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
                  } else if (strValue == "analog") {
                     
                  } else if (strValue == "virtual") {
                     
                  } else {
                     printf(F("invalid pin mode!"));
                     nExitValue = EXIT_FAILURE;
                  }
               } else if (nValue < 1024) {
                  CxGPIODevice* pDev = _gpioDeviceManager.getDeviceByPin(nPin);
                  if (pDev) pDev->set(nValue);
               } else {
                  printf(F("invalid value!"));
                  nExitValue = EXIT_FAILURE;
               }
            } else {
               nExitValue = EXIT_FAILURE;
               println("invalid");
               __gpioTracker.printInvalidReason(getIoStream(), nPin);
            }
         } else if (strSubCmd == "get") {
            if (__gpioTracker.isValidPin(nPin)) {
               CxGPIO gpio(nPin);
               gpio.printState(getIoStream());
               nExitValue = EXIT_SUCCESS;
            } else {
               __gpioTracker.printInvalidReason(getIoStream(), nPin);
            }
         } else if (strSubCmd == "list") {
            _gpioDeviceManager.printList();
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "add") {
            if (nPin != INVALID_PIN) {
               String strType = TKTOCHAR(tkArgs, 3);
               String strName = TKTOCHAR(tkArgs, 4);
               bool bInverted = TKTOINT(tkArgs, 5, false);
               String strGpioCmd = TKTOCHAR(tkArgs, 6);
               bool bPullup = TKTOINT(tkArgs, 7, false);
               if (strType == "button") {
                  // FIXME: pointer without proper deletion? even if managed internally? maybe container as for the bme?
                  /// TODO: consider dyanmic cast to ensure correct type
                  CxButton* pButton = static_cast<CxButton*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pButton) {
                     pButton->setName(strName.c_str());
                     pButton->setInverted(bInverted);
                     pButton->setCmd(strGpioCmd.c_str());
                     pButton->begin();
                     nExitValue = EXIT_SUCCESS;
                  } else {
                     if (strGpioCmd == "reset") {
                        CxButtonReset* p = new CxButtonReset(nPin, strName.c_str(), bInverted, bPullup);
                        if (p) {
                           p->begin();
                           nExitValue = EXIT_SUCCESS;
                        }
                     } else {
                        CxButton* p = new CxButton(nPin, strName.c_str(), bInverted, bPullup, strGpioCmd.c_str());
                        if (p) {
                           p->begin();
                           nExitValue = EXIT_SUCCESS;
                        }
                     }
                  }
               } else if (strType == "led") {
                  if (strName == "led1") {
                     Led1.setPin(nPin);
                     Led1.setPinMode(OUTPUT);
                     Led1.setName(strName.c_str());
                     Led1.setInverted(bInverted);
                     Led1.setCmd(strGpioCmd.c_str());
                     Led1.off();
                     nExitValue = EXIT_SUCCESS;
                  } else {
                     CxLed* p = static_cast<CxLed*>(_gpioDeviceManager.getDeviceByPin(nPin));
                     if (p) {
                        p->setName(strName.c_str());
                        p->setInverted(bInverted);
                        p->setCmd(strGpioCmd.c_str());
                        p->begin();
                        p->off();
                        nExitValue = EXIT_SUCCESS;
                     } else {
                        CxLed* p = new CxLed(nPin, strName.c_str(), bInverted);
                        if (p) {
                           //p->setFriendlyName();
                           p->begin();
                           nExitValue = EXIT_SUCCESS;
                        }
                     }
                  }
               } else if (strType == "relay") {
                  /// TODO: consider dyanmic cast to ensure correct type
                  CxRelay* pRelay = static_cast<CxRelay*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pRelay) {
                     pRelay->setName(strName.c_str());
                     pRelay->setInverted(bInverted);
                     pRelay->setCmd(strGpioCmd.c_str());
                     pRelay->begin();
                     nExitValue = EXIT_SUCCESS;
                  }  else {
                     CxRelay* p = new CxRelay(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
                     if (p) {
                        p->begin();
                        nExitValue = EXIT_SUCCESS;
                     }
                  }
               } else if (strType == "contact") {
                  CxContact* pContact = static_cast<CxContact*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pContact) {
                     pContact->setName(strName.c_str());
                     pContact->setInverted(bInverted);
                     pContact->setCmd(strGpioCmd.c_str());
                     pContact->begin();
                     nExitValue = EXIT_SUCCESS;
                  } else {
                     CxContact* p = new CxContact(nPin, strName.c_str(), bInverted, bPullup, strGpioCmd.c_str());
                     if (p) {
                        p->begin();
                        nExitValue = EXIT_SUCCESS;
                     }
                  }
               } else if (strType == "counter") {
                  CxCounter* pCounter = static_cast<CxCounter*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pCounter) {
                     pCounter->setName(strName.c_str());
                     pCounter->setInverted(bInverted);
                     pCounter->setCmd(strGpioCmd.c_str());
                     pCounter->begin();
                     nExitValue = EXIT_SUCCESS;
                  } else {
                     CxCounter* p = new CxCounter(nPin, strName.c_str(), bInverted, bPullup, strGpioCmd.c_str());
                     if (p) {
                        p->begin();
                        nExitValue = EXIT_SUCCESS;
                     }
                  }
               } else if (strType == "analog") {
                  CxAnalog* pAnalog = static_cast<CxAnalog*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pAnalog) {
                     pAnalog->setName(strName.c_str());
                     pAnalog->setInverted(bInverted);
                     pAnalog->setCmd(strGpioCmd.c_str());
                     pAnalog->setTimer(TKTOINT(tkArgs, 7, 1000)); // default update rate 1s
                     pAnalog->begin();
                     nExitValue = EXIT_SUCCESS;
                  } else {
                     CxAnalog* p = new CxAnalog(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
                     if (p) {
                        p->begin();
                        nExitValue = EXIT_SUCCESS;
                     }
                  }
               } else if (strType == "virtual") {
                  CxGPIOVirtual* pVirtual = static_cast<CxGPIOVirtual*>(_gpioDeviceManager.getDeviceByName(strName.c_str()));
                  if (pVirtual) {
                     pVirtual->setName(strName.c_str());
                     pVirtual->setInverted(bInverted);
                     pVirtual->setCmd(strGpioCmd.c_str());
                     pVirtual->begin();
                     nExitValue = EXIT_SUCCESS;
                  } else {
                     CxGPIOVirtual* p = new CxGPIOVirtual(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
                     if (p) {
                        p->begin();
                        nExitValue = EXIT_SUCCESS;
                     }
                  }
               }
               else {
                  println(F("invalid device type!"));
               }
            } else {
               println(F("invalid pin!"));
            }
         } else if (strSubCmd == "del") {
            // FIXME: delete command crashes the system
            String strName = TKTOCHAR(tkArgs, 2);
            if (strName == "led1") {
               Led1.setPin(INVALID_PIN);
               Led1.setName("");
               nExitValue = EXIT_SUCCESS;
            } else {
               CxGPIODevice* p = _gpioDeviceManager.getDevice(strName.c_str());
               nExitValue = EXIT_SUCCESS;
               if (p) {
                  delete p;
               } else {
                  println(F("device not found!"));
                  nExitValue = EXIT_FAILURE;
               }
               _gpioDeviceManager.removeDevice(strName.c_str());
            }
         } else if (strSubCmd == "name") {
            if (__gpioTracker.isValidPin(nPin)) {
               CxGPIODevice* p = _gpioDeviceManager.getDeviceByPin(nPin);
               if (p) {
                  p->setFriendlyName(strValue.c_str());
                  p->setName(strValue.c_str());
                  nExitValue = EXIT_SUCCESS;
               } else {
                  println(F("device not found!"));
               }
            } else {
               println(F("invalid pin!"));
            }
         } else if (strSubCmd == "fn") {
            CxGPIODevice* p = _gpioDeviceManager.getDeviceByPin(nPin);
            
            if (p) {
               p->setFriendlyName(TKTOCHAR(tkArgs, 3));
               nExitValue = EXIT_SUCCESS;
            } else {
               println(F("device not found!"));
            }
            
         } else if (strSubCmd == "deb") {
            CxGPIODevice* p = _gpioDeviceManager.getDeviceByPin(nPin);
            
            if (p) {
               p->setDebounce(TKTOINT(tkArgs, 3, p->getDebounce()));
               nExitValue = EXIT_SUCCESS;
            } else {
               println(F("device not found!"));
            }
         } else if (strSubCmd == "isr") {
            // isr <pin> <id> [<debounce time>]
            CxGPIODevice* p = _gpioDeviceManager.getDeviceByPin(nPin);
            if (p) {
               p->setDebounce(TKTOINT(tkArgs, 4, p->getDebounce()));
               p->setISR(TKTOINT(tkArgs, 3, INVALID_UINT8));
               p->enableISR();
               nExitValue = EXIT_SUCCESS;
            } else {
               CxTablePrinter table(getIoStream());
               table.printHeader({F("ID"), F("Counter"), F("Debounce")}, {3, 10, 8});
               for (int i = 0; i < 3; i++) {
                  table.printRow({String(i).c_str(), String(g_anEdgeCounter[i]).c_str(), String(g_anDebounceDelay[i]).c_str()});
               }
               nExitValue = EXIT_SUCCESS;
            }
         }
         else if (strSubCmd == "let" && tkArgs.count() > 4) {
            String strOperator = TKTOCHAR(tkArgs, 3);
            CxGPIODevice* dev1 = _gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 2));
            CxGPIODevice* dev2 = _gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 4));
            
            if (strOperator == "=") {
               if (dev1 && dev2) {
                  dev1->set(dev2->get());
                  nExitValue = EXIT_SUCCESS;
               } else if (dev1) {
                  String strValue = TKTOCHAR(tkArgs, 4);
                  uint32_t nValue = INVALID_UINT32;
                  char* end = nullptr;

                  if (strValue.startsWith("$")) {  // we need this? substitution done on higher level actually
                     __console.substituteVariables(strValue);
                  }

                  nValue = (uint32_t)std::strtol(strValue.c_str(), &end, 0); // return as uint32_t with auto base
                  
                  // Check if the conversion failed (no characters processed or out of range)
                  if (!end || end == strValue.c_str() || *end != '\0') {
                     __console.error(F("cannot assign the value %s to %s (not a number)"), strValue.c_str(), dev1->getName());
                  } else {
                     dev1->set((bool)nValue); // MARK: currently only bool is supported
                     nExitValue = EXIT_SUCCESS;
                  }
               } else {
                  println(F("device not found!"));
               }
            }
         }
         else {
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("gpio commands:"));
               println(F("  state [<pin>]"));
               println(F("  set <pin> <mode> (in, out, pwm, inverted, non-inverted"));
               println(F("  set <pin> 0...1023 (set pin state to value)"));
               println(F("  name <pin> <name>"));
               println(F("  fn <pin> <friendly name>"));
               println(F("  get <pin>"));
               println(F("  list"));
               println(F("  add <pin> <type> <name> <inverted> [<cmd> [<param>]]")); // param: bPullup (input) or period (analog)
               println(F("  del <name>"));
               println(F("  let <name> = <name>"));
               println(F("  isr <pin> <id> [<debounce time>]"));
#endif
            }
         }
      } else if (cmd == "led") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         uint8_t nIndexOffset = 0;
         
         CxLed* led = &Led1;
         
         CxLed* p = static_cast<CxLed*>(_gpioDeviceManager.getDeviceByName(TKTOCHAR(tkArgs, 1)));
         
         if (p) {
            led = p;
            strSubCmd = TKTOCHAR(tkArgs, 2);
            nIndexOffset = 1;
         }
         
         strSubCmd.toLowerCase();

         if (strSubCmd == "on") {
            led->on();
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "off") {
            led->off();
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "blink") {
            String strPattern = TKTOCHAR(tkArgs, 2+nIndexOffset);
            if (strPattern == "ok") {
               led->blinkOk();
            } else if (strPattern == "error") {
               led->blinkError();
            } else if (strPattern == "busy") {
               led->blinkBusy();
            } else if (strPattern == "flash") {
               led->blinkFlash();
            } else if (strPattern == "data") {
               led->blinkData();
            } else if (strPattern == "wait") {
               led->blinkWait();
            } else if (strPattern == "connect") {
               led->blinkConnect();
            }  else {
               led->setBlink(TKTOINT(tkArgs, 2+nIndexOffset, 1000), TKTOINT(tkArgs, 3+nIndexOffset, 128));
            }
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "flash") {
            String strPattern = TKTOCHAR(tkArgs, 2+nIndexOffset);
            if (strPattern == "ok") {
               led->flashOk();
            } else if (strPattern == "error") {
               led->flashError();
            } else if (strPattern == "busy") {
               led->flashBusy();
            } else if (strPattern == "flash") {
               led->flashFlash();
            } else if (strPattern == "data") {
               led->flashData();
            } else if (strPattern == "wait") {
               led->flashWait();
            } else if (strPattern == "connect") {
               led->flashConnect();
            } else {
               led->setFlash(TKTOINT(tkArgs, 2+nIndexOffset, 250), TKTOINT(tkArgs, 3+nIndexOffset, 128), TKTOINT(tkArgs, 4+nIndexOffset, 1));
            }
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "invert") {
            if (b) {
               led->setInverted(TKTOINT(tkArgs, 2+nIndexOffset, false));
            } else {
               led->setInverted(!led->isInverted());
               led->toggle();
            }
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "toggle") {
            led->toggle();
            nExitValue = EXIT_SUCCESS;
         }
         else {
            printf(F("LED on pin %02d%s\n"), led->getPin(), led->isInverted() ? ",inverted":"");
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("usage: led [name] <command>"));
               println(F("commands: "));
               println(F("  on|off"));
               println(F("  toggle"));
               println(F("  blink [period] [duty]"));
               println(F("  blink [pattern] (ok, error...)"));
               println(F("  flash [period] [duty] [number]"));
               println(F("  invert [0|1]"));
#endif
            }
         }
      } else if (cmd == "sensor") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         if (strSubCmd == "list") {
            _sensorManager.printList();
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "name") {
            uint8_t nId = TKTOINT(tkArgs, 2, INVALID_UINT8);
            if (nId != INVALID_UINT8) {
               _sensorManager.setSensorName(nId, TKTOCHAR(tkArgs, 3));
               nExitValue = EXIT_SUCCESS;
            } else {
               println(F("usage: sensor name <id> <name>"));
            }
         } else if (strSubCmd == "get") {
            float f = _sensorManager.getSensorValueFloat(TKTOINT(tkArgs, 2, INVALID_FLOAT));
            if (!std::isnan(f)) {
               println(f);
               nExitValue = EXIT_SUCCESS;
               __console.setOutputVariable(f);
            } else {
               println(F("invalid sensor id!"));
            }
         } else if (strSubCmd == "add" && tkArgs.count() > 5) {
            // sensor add <name> <type> <unit> <variable>
            String strVar = TKTOCHAR(tkArgs, 5);
            CxSensor* pSensor = _sensorManager.getSensor(TKTOCHAR(tkArgs, 2));
            
            if (pSensor) {
               nExitValue = EXIT_SUCCESS;
            } else {
               String strType =TKTOCHAR(tkArgs, 3);
               pSensor = new CxSensorGeneric(TKTOCHAR(tkArgs, 2), ECSensorType::other, TKTOCHAR(tkArgs, 4), [this, strVar]()->float {
                  if (strVar.length() > 0) {
                     float fValue = 0.0F;
                     char* end = nullptr;
                     const char* szValue = __console.getVariable(strVar.c_str());
                     if (szValue) {
                        fValue = std::strtod(szValue, &end); // return as uint32_t with auto base
                     }
                     
                     // Check if the conversion failed (no characters processed or out of range), then concat two strings
                     if (end && end != szValue && *end == '\0') {
                        return fValue;
                     }
                  }
                  return INVALID_FLOAT;
               });
               if (pSensor) {
                  pSensor->setTypeSz(strType.c_str());
                  pSensor->setFriendlyName(TKTOCHAR(tkArgs, 6));
                  nExitValue = EXIT_SUCCESS;
               }
            }
         } else if (strSubCmd == "del") {
            _sensorManager.removeSensor(TKTOCHAR(tkArgs, 2));
            nExitValue = EXIT_SUCCESS;
         }
         else {
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("sensor commands:"));
               println(F("  list"));
               println(F("  name <id> <name>"));
               println(F("  get <id>"));
               println(F("  add <name> <unit> <variable> [<friendly name>]"));
#endif
            }
         }
      } else if (cmd == "relay") {
         String strName = TKTOCHAR(tkArgs, 1);
         String strSubCmd = TKTOCHAR(tkArgs, 2);
         strSubCmd.toLowerCase();
         
         CxGPIODevice* pDev = _gpioDeviceManager.getDevice(strName.c_str());
         
         if (strName == "list") {
            _gpioDeviceManager.printList("relay");
         } else if (pDev) {
            String strType = pDev->getTypeSz();
            
            if (strType != "relay") {
               __console.println(F("device is not a relay!"));
               nExitValue = EXIT_SUCCESS;
            } else {
               CxRelay* p = static_cast<CxRelay*>(_gpioDeviceManager.getDevice(strName.c_str()));
               nExitValue = EXIT_SUCCESS;

               if (strSubCmd == "on") {
                  p->on();
               } else if (strSubCmd == "off") {
                  p->off();
               } else if (strSubCmd == "toggle") {
                  p->toggle();
               } else if (strSubCmd == "offtimer") {
                  p->setOffTimer(TKTOINT(tkArgs, 3, 0));
               } else if (strSubCmd == "default") {
                  p->setDefaultOn(TKTOINT(tkArgs, 3, 0));
               }
               else {
                  nExitValue = EXIT_FAILURE;
                  __console.println(F("invalid relay command"));
               }
            }
         } else {
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("relay commands:"));
               println(F("  list")); // TODO: list relays
               println(F("  <name> on"));
               println(F("  <name> off"));
               println(F("  <name> toggle"));
               println(F("  <name> offtimer <ms>"));
               println(F("  <name> default <0|1>"));
#endif
            }
         }
      } else if (cmd == "processdata") {
         String strType = TKTOCHAR(tkArgs, 1);
         if (strType == "json" && tkArgs.count() > 3) {
            if (!_mapProcessJsonDataItems.size()) {
               // register processdata method with the first set data item
               __console.setFuncProcessData([this](const char* data)->bool{
                  
                  // prints the received data to the console
                  __console.printLog(LOGLEVEL_DEBUG_EXT, DEBUG_FLAG_USER, data);
                  bool bSucccess = false;
                  
                  const char* jsonState = __console.getVariable("jsonstate");

                  DynamicJsonDocument doc(1024);
                  DeserializationError error = deserializeJson(doc, data);
                  if (!error) {
                     bool bValid = true;
                     
                     if (jsonState) {
                        const char* pszJsonState = getJsonValueSz(doc, jsonState, "true");
                        if (pszJsonState && strcmp(pszJsonState, "false") == 0) {
                           bValid = false;
                        }
                     }
                     
                     if (bValid) {
                        for (const auto& pair : _mapProcessJsonDataItems) {
                           const char* szJsonValue = getJsonValueSz(doc, pair.first.c_str(), "");
                           if (szJsonValue) {
                              String strCmd = pair.second;
                              _CONSOLE_DEBUG_EXT(DEBUG_FLAG_DATA_PROC, F("process json data %s = %s"), pair.first.c_str(), szJsonValue);
                              strCmd.replace(F("$(VALUE)"), szJsonValue);
                              __console.processCmd(strCmd.c_str());
                           }
                        }
                        bSucccess = true;
                     } else {
                        _CONSOLE_DEBUG_EXT(DEBUG_FLAG_DATA_PROC, F("json state is false, stop processing the data"));
                     }
                  } else {
                     __console.error(F("json data de-serialisation error!"));
                  }
                  return true;
               });
            }
            _mapProcessJsonDataItems[TKTOCHAR(tkArgs, 2)] = TKTOCHAR(tkArgs, 3);
            nExitValue = EXIT_SUCCESS;
         } else if (strType == "list") {
            CxTablePrinter table(getIoStream());
            table.printHeader({F("Json Path"), F("Command")}, {20, 40});
            for (const auto& pair : _mapProcessJsonDataItems) {
               table.printRow({pair.first, pair.second.c_str()});
            }
            nExitValue = EXIT_SUCCESS;
         }
      } else if (cmd == "smooth") {
         // smooth <reference> <value> <maxDiff> [<threshold> <minAlpha> <maxAlpha>]
         // set the output variable $> to the smoothed value
         // sets the exit value to 0, if valid
         // test data:
         // smooth 100   106   10   5   0.1   1.0 ; echo $>   #106 (outlier rejected, diff=6 > maxDiff=1)
         // smooth 100   101   10   5   0.1   1.0 ; echo $>   #100.28 (small diff → smooth partial update)
         // smooth  50    52    3   2   0.2   0.7 ; echo $>   #51.4 partial smoothing
         // smooth 200   195   10   5   0.05  0.5 ; echo $>   #197.5 small diff, low alpha smoothing
         // smooth 200   185   10   5   0.05  0.5 ; echo $>   #200, outlier
         // smooth 100   110   10   5   0.1   1.0 ; echo $>   #110 (diff=10 == maxDiff, full update)
         // smooth   0     0    1   0   0.1   0.4 ; echo $>   #0 (no change)
         // smooth   0     1    1   0   0.1   0.4 ; echo $>   #0.1 (fixed alpha smoothing since threshold=0)
         // smooth 100   105    5 ; echo $>                   #105 (no smoothing, accepted value)
         
         float reference = TKTOFLOAT(tkArgs, 1, INVALID_FLOAT);
         float value = TKTOFLOAT(tkArgs, 2, INVALID_FLOAT);
         float maxDiff = TKTOFLOAT(tkArgs, 3, INVALID_FLOAT);
         float threshold = TKTOFLOAT(tkArgs, 4, INVALID_FLOAT);
         float minAlpha = TKTOFLOAT(tkArgs, 5, INVALID_FLOAT);
         float maxAlpha = TKTOFLOAT(tkArgs, 6, INVALID_FLOAT);
                  
         if (std::isnan(value) || std::isnan(maxDiff)) {
            
         } else {
            float fValue = smoothRobust(reference, value, maxDiff, threshold, minAlpha, maxAlpha);
            nExitValue = (std::isnan(fValue)?1:0); // consider as non success (exit code 1), if nan was returned.
            __console.setOutputVariable(fValue);
         }
         nExitValue = EXIT_SUCCESS;
      }
      else {
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }
   
   const char* getJsonValueSz(const JsonDocument& doc, const char* path, const char* defaultValue) {
      JsonVariant var = const_cast<JsonDocument&>(doc);
      char buf[64];
      strncpy(buf, path, sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
      
      char* token = strtok(buf, ".");
      while (token && var.is<JsonObject>()) {
         var = var[token];
         token = strtok(nullptr, ".");
      }
      if (var.isNull()) return defaultValue;
      
      static char result[32];
      if (var.is<const char*>()) {
         return var.as<const char*>();
      } else if (var.is<bool>()) {
         snprintf(result, sizeof(result), "%s", var.as<bool>() ? "true" : "false");
         return result;
      } else if (var.is<int>()) {
         snprintf(result, sizeof(result), "%d", var.as<int>());
         return result;
      } else if (var.is<float>()) {
         snprintf(result, sizeof(result), "%g", var.as<float>());
         return result;
      }
      return defaultValue;
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
      printf(F(ESC_ATTR_BOLD " Core:" ESC_ATTR_RESET " %s\n"), ESP.getCoreVersion().c_str());
      printf(F(ESC_ATTR_BOLD "    SDK:" ESC_ATTR_RESET " %s"), ESP.getSdkVersion());
      
      
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
      printf(F(ESC_ATTR_BOLD " Arduino:" ESC_ATTR_RESET " %d.%d.%d %s\n"), major, minor, patch, ide);
#endif
      printf(F(ESC_ATTR_BOLD "    Firmware:" ESC_ATTR_RESET " %s" ESC_ATTR_BOLD " Ver.:" ESC_ATTR_RESET " %s"), __console.getAppName(), __console.getAppVer());
#ifdef ARDUINO
      printf(F(ESC_ATTR_BOLD " Sketch size: " ESC_ATTR_RESET));
      if (ESP.getSketchSize()/1024 < 465) {
         printf(F( "%d kBytes\n"), ESP.getSketchSize()/1024);
      } else if (ESP.getSketchSize()) {
         printf(F(ESC_TEXT_BRIGHT_YELLOW ESC_ATTR_BOLD "%d kBytes\n"), ESP.getSketchSize()/1024);
      } else if (getFreeOTA() < ESP.getSketchSize()) {
         printf(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BOLD "%d kBytes\n"), ESP.getSketchSize()/1024);
      }
      print(ESC_ATTR_RESET);
#else
      println();
#endif
   }
   
   void printESP() {
#ifndef MINIMAL_COMMAND_SET
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
      printf(F("Size avail.:  %5d kBytes\n"), (ESP.getSketchSize() + ESP.getFreeSketchSpace())/1024);
      printf(F("     sketch:  %5d kBytes\n"), (ESP.getSketchSize()/1024));
      printf(F("       free:  %5d kBytes\n"), (ESP.getFreeSketchSpace()/1024));
#ifdef ESP32
      printf(F("   OTA room:  ? Bytes\n"));
#else
      printf(F("   OTA room:  %5d kBytes\n"), getFreeOTA()/1024);
      if (getFreeOTA() < ESP.getSketchSize()) {
         printf(F("*** Free room for OTA too low!\n"));
      } else if (getFreeOTA() < (ESP.getSketchSize() + 10000)) {
         printf(F("vvv Free room for OTA is getting low!\n"));
      }
      printf(F("FLASHFS size: %5d kBytes\n"), getFSSize()/1024);
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
      printf(F("Application:  %s (%s)\n"), __console.getAppName(), __console.getAppVer());
      printf(F("\n"));
      printf(F("-BOOT-------------------\n"));
      printf(F("reset reason: %s\n"), getResetInfo());
      print(F("time to boot: ")); __console.printTimeToBoot(getIoStream()); println();
      printf(F("free heap:    %5d Bytes\n"), ESP.getFreeHeap());
      printf(F("\n"));
#endif
#else
      println(F("red. cmd set"));
#endif /* MINIMAL_COMMAND_SET*/

   }

   void printFlashMap() {
#ifndef MINIMAL_COMMAND_SET

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
      printf(F("OTA start:    %X (lowest possible addr.)\n"), getOTAStart());
      printf(F("OTA end:      %X (%d kBytes available)\n"), getOTAEnd(), getFreeOTA()/1024);
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
#else
      println(F("red. cmd set"));

#endif /*MINIMAL_COMMAND_SET*/

   }
   
#ifndef ESP_CONSOLE_NOWIFI
   bool checkWifi() {
#ifdef ARDUINO
      bool bConnected = (WiFi.status() == WL_CONNECTED);
      
      if (_bWifiConnected != bConnected) {
         _bWifiConnected = bConnected;
         if (bConnected) {
            __console.executeBatch("init", "wifi-online");
         } else {
            __console.executeBatch("init", "wifi-offline");
         }
      }
      return bConnected;
#else
      return false;
#endif
   }

   bool isHostAvailble(const char* host, uint32_t port) {
#ifdef ARDUINO
      if (WiFi.status() == WL_CONNECTED && port && host) { //Check WiFi connection status
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
   
   void gpioAction() {
      // check for gpio events
      _gpioDeviceManager.loop(__console.isAPMode());
   }

   void startWiFi(const char* ssid = nullptr, const char* pw = nullptr) {
      bool bUp = false;
      
#ifndef ESP_CONSOLE_NOWIFI
      _stopAP();
      
      if (checkWifi()) {
         stopWiFi();
      }
      
      if (!bUp) {
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
         
         static char szSSID[20];
         static char szPassword[25];
         static char szHostname[80];
         
         if (ssid) ::writeSSID(ssid);
         ::readSSID(szSSID, sizeof(szSSID));
         
         if (pw) ::writePassword(pw);
         ::readPassword(szPassword, sizeof(szPassword));
         
         ::readHostName(szHostname, sizeof(szHostname));
         
#ifdef ARDUINO
         WiFi.persistent(false); // Disable persistent WiFi settings, preventing flash wear and keep control of saved settings
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
         
         // stop blinking "..." and let the message on the screen
         print(ESC_CLEAR_LINE "\r");
         printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s..." ESC_ATTR_RESET), szSSID);
         
         Led1.off();
         
         if (WiFi.status() != WL_CONNECTED) {
            _bWifiConnected = true;
            println(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "not connected!" ESC_ATTR_RESET));
            __console.error("WiFi not connected.");
            Led1.blinkError();
         } else {
            println(F(ESC_TEXT_BRIGHT_GREEN "connected!" ESC_ATTR_RESET));
            _CONSOLE_INFO("WiFi connected.");
            Led1.flashOk();
            if (WiFi.getHostname() != szHostname) {
#ifdef ESP32
               __console.setHostName(WiFi.getHostname().c_str());
#else
               __console.setHostName(WiFi.hostname().c_str());
#endif
            }
            
            bUp = true;
         }
         
#endif /* Arduino */
         
      }
      
      if (bUp) {
         __console.executeBatch("init", "wifi-up");
         checkWifi();
      }
   }
   
   void stopWiFi() {
      _CONSOLE_INFO(F("WiFi disconnect and switch off."));
      println(F("WiFi disconnect and switch off."));
#ifdef ARDUINO
      WiFi.disconnect();
      WiFi.softAPdisconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
#endif
      checkWifi();
      __console.executeBatch("init", "wifi-down");
   }
   
private:
   /// handle the root request from the web client to provide a captive portal for wifi connection.
   static void _handleRoot() {
#ifdef ARDUINO
      String htmlPage;

#if defined(CxCapabilityFS_hpp) && !defined(ESP_CONSOLE_NOWIFI)
      File file = LittleFS.open("/ap.html", "r");
      if (!file) {
         webServer.send(404, "text/plain", "HTML file not found");
         return;
      }
      
      htmlPage = file.readString();
      file.close();
#else
      htmlPage = htmlPageTemplate;
#endif /* CxCapabilityFS_hpp */
      
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
#endif /* ARDUINO */
   }

   /// Handle the connect request from the captive portal to connect to a WiFi network.
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
   
   /// access point mode
   void _beginAP() {
      _CONSOLE_INFO(F("Starting Access Point..."));
      
      stopWiFi();
      
      Led1.blinkWait();
      
#ifdef ARDUINO
      WiFi.forceSleepWake(); ///< wake up the wifi chip
      delay(100);
      WiFi.persistent(false); ///< Disable persistent WiFi settings, preventing flash wear and keep control of saved settings
      WiFi.mode(WIFI_AP);  ///< Set WiFi mode to AP
      
      /// Start the Access Point with the given hostname and password
      if (WiFi.softAP(__console.getHostName(), "12345678")) {
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
         _CONSOLE_INFO(F("ESP started in AP mode"));
         printf(F("ESP started in AP mode. SSID: %s, PW: %s, IP: %s\n"), __console.getHostName(), "12345678", WiFi.softAPIP().toString().c_str());
         
         __console.setAPMode(true);
         __console.executeBatch("init", "ap-up");

      } else {
         __console.error(F("Failed to start Access Point, going back to STA mode"));
         startWiFi();
      }
#endif
   }

   /// stopping the access point mode
   void _stopAP() {
      Led1.off();
      
#if defined(ESP32)
      webServer.stop();
#elif defined(ESP8266)
      webServer.close();
      dnsServer.stop();
#endif
      __console.setAPMode(false);
      __console.executeBatch("init", "ap-down");
   }


#endif /* ESP_CONSOLE_NOWIFI */

public:
   static void loadCap() {
      CAPREG(CxCapabilityExt);
      CAPLOAD(CxCapabilityExt);
   };
};



#endif /* CxCapabilityExt */

