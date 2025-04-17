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
 * - CxConfigParser.hpp: Configuration parser class
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
   
   /// timer for updating stack info and sensor data
   CxTimer10s _timerUpdate;
   
public:
   /// Default constructor and default capabilities methods
   explicit CxCapabilityExt() : CxCapability("ext", getCmds()) {}
   static constexpr const char* getName() { return "ext"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "hw", "sw", "esp", "flash", "set", "eeprom", "wifi", "gpio", "led", "ping", "sensor", "relay" };
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
      
      if (!isConnected()) {
         println();
         startWiFi();
      }

      /// led status indication
      Led1.off();
      if (isConnected()) {
         Led1.flashOk();
      } else {
         Led1.blinkError();
      }
      
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

      /// check gpio events
      gpioAction();
      
      /// update sensor data and stack info
      if (_timerUpdate.isDue()) {
         g_Heap.update();
         _sensorManager.update();
      }
      
   }
   
   /// Execute method to process the given command and return the result.
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
         /// fix env variables:
         /// - NTP <server>
         /// - TZ <timezone>
         ///
         
         String strVar = TKTOCHAR(tkArgs, 1);
         if (strVar == "NTP") {
            __console.setNtpServer(TKTOCHAR(tkArgs, 2));
            __console.addVariable(strVar.c_str(), TKTOCHAR(tkArgs, 2));
         } else if (strVar == "TZ") {
            __console.setTimeZone(TKTOCHAR(tkArgs, 2));
            __console.addVariable(strVar.c_str(), TKTOCHAR(tkArgs, 2));
         } else if (strVar.length() > 0) {
            String strValue = TKTOCHAR(tkArgs, 2);
            
            if (strValue.length() > 0) {
               __console.addVariable(strVar.c_str(), strValue.c_str());
            } else {
               __console.removeVariable(strVar.c_str());
            }
         } else {
            /// print all variables
            __console.printVariables(getIoStream());
         }
      } else if (cmd == "eeprom") {
         if (a) {
            ::printEEPROM(getIoStream(), TKTOINT(tkArgs, 1, 0), TKTOINT(tkArgs, 2, 128));
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
               __console.setHostName(TKTOCHAR(tkArgs, 2));
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
            __console.processCmd("net");
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
         } else {
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
#endif
            }
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
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         uint8_t nPin = TKTOINT(tkArgs, 2, INVALID_PIN);
         int16_t nValue = TKTOINT(tkArgs, 3, -1);
         String strValue = TKTOCHAR(tkArgs, 3);
         String strEnv = ".gpio";
         
         if (strSubCmd == "state") {
            _gpioTracker.printAllStates(getIoStream());
         } else if (strSubCmd == "set") {
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
         } else if (strSubCmd == "get") {
            if (CxGPIO::isValidPin(nPin)) {
               CxGPIO gpio(nPin);
               if (gpio.isSet()) {
                  gpio.printState(getIoStream());
               }
            } else {
               CxGPIO::printInvalidReason(getIoStream(), nPin);
            }
         } else if (strSubCmd == "list") {
            _gpioDeviceManager.printList();
         } else if (strSubCmd == "add") {
            if (nPin != INVALID_PIN) {
               String strType = TKTOCHAR(tkArgs, 3);
               String strName = TKTOCHAR(tkArgs, 4);
               bool bInverted = TKTOINT(tkArgs, 5, false);
               String strGpioCmd = TKTOCHAR(tkArgs, 6);
               if (strType == "button") {
                  // FIXME: pointer without proper deletion? even if managed internally? maybe container as for the bme?
                  /// TODO: consider dyanmic cast to ensure correct type
                  CxButton* pButton = static_cast<CxButton*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pButton) {
                     //pButton->setFriendlyName();
                     pButton->setName(strName.c_str());
                     pButton->setInverted(bInverted);
                     pButton->setCmd(strGpioCmd.c_str());
                     pButton->begin();
                  } else {
                     if (strGpioCmd == "reset") {
                        CxButtonReset* p = new CxButtonReset(nPin, strName.c_str(), bInverted);
                        if (p) {
                           //p->setFriendlyName();
                           p->begin();
                        }
                     } else {
                        CxButton* p = new CxButton(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
                        if (p) {
                           //p->setFriendlyName();
                           p->begin();
                        }
                     }
                  }
               } else if (strType == "led") {
                  if (strName == "led1") {
                     Led1.setPin(nPin);
                     Led1.setPinMode(OUTPUT);
                     Led1.setName(strName.c_str());
                     //Led1.setFriendlyName();
                     Led1.setInverted(bInverted);
                     Led1.setCmd(strGpioCmd.c_str());
                     Led1.off();
                  } else {
                     CxLed* p = static_cast<CxLed*>(_gpioDeviceManager.getDeviceByPin(nPin));
                     if (p) {
                        //p->setFriendlyName();
                        p->setName(strName.c_str());
                        p->setInverted(bInverted);
                        p->setCmd(strGpioCmd.c_str());
                        p->begin();
                        p->off();
                     } else {
                        CxLed* p = new CxLed(nPin, strName.c_str(), bInverted);
                        if (p) {
                           //p->setFriendlyName();
                           p->begin();
                        }
                     }
                  }
               } else if (strType == "relay") {
                  /// TODO: consider dyanmic cast to ensure correct type
                  CxRelay* pRelay = static_cast<CxRelay*>(_gpioDeviceManager.getDeviceByPin(nPin));
                  if (pRelay) {
                     //pRelay->setFriendlyName();
                     pRelay->setName(strName.c_str());
                     pRelay->setInverted(bInverted);
                     pRelay->setCmd(strGpioCmd.c_str());
                     pRelay->begin();
                  }  else {
                     CxRelay* p = new CxRelay(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
                     if (p) {
                        //p->setFriendlyName();
                        p->begin();
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
            } else {
               CxDevice* p = _gpioDeviceManager.getDevice(strName.c_str());
               if (p) {
                  delete p;
               } else {
                  println(F("device not found!"));
               }
               _gpioDeviceManager.removeDevice(strName.c_str());
            }
         } else if (strSubCmd == "name") {
            if (CxGPIO::isValidPin(nPin)) {
               CxDevice* p = _gpioDeviceManager.getDeviceByPin(nPin);
               if (p) {
                  p->setFriendlyName(strValue.c_str());
                  p->setName(strValue.c_str());
               } else {
                  println(F("device not found!"));
               }
            } else {
               println(F("invalid pin!"));
            }
         } else if (strSubCmd == "fn") {
            CxDevice* p = _gpioDeviceManager.getDeviceByPin(nPin);
            
            if (p) {
               p->setFriendlyName(TKTOCHAR(tkArgs, 3));
            } else {
               println(F("device not found!"));
            }

         }
         else if (strSubCmd == "let") {
            String strOpertor = TKTOCHAR(tkArgs, 3);
            CxDevice* dev1 = _gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 2));
            CxDevice* dev2 = _gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 4));
            
            if (dev1 && dev2) {
               if (strOpertor == "=") {
                  dev1->set(dev2->get());
               }
            } else {
               println(F("device not found!"));
            }
         }
         else {
            _gpioTracker.printAllStates(getIoStream());
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
               println(F("  add <pin> <type> <name> <inverted> [<cmd>]"));
               println(F("  del <name>"));
               println(F("  let <name> = <name>"));
#endif
            }
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
            if (b) {
               Led1.setInverted(TKTOINT(tkArgs, 2, false));
            } else {
               Led1.setInverted(!Led1.isInverted());
               Led1.toggle();
            }
         } else {
            printf(F("LED on pin %02d%s\n"), Led1.getPin(), Led1.isInverted() ? ",inverted":"");
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("led commands:"));
               println(F("  on|off"));
               println(F("  blink [period] [duty]"));
               println(F("  blink [pattern] (ok, error...)"));
               println(F("  flash [period] [duty] [number]"));
               println(F("  invert [0|1]"));
#endif
            }
         }
      } else if (cmd == "sensor") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         String strEnv = ".sensors";
         if (strSubCmd == "list") {
            _sensorManager.printList();
         } else if (strSubCmd == "name") {
            uint8_t nId = TKTOINT(tkArgs, 2, INVALID_UINT8);
            if (nId != INVALID_UINT8) {
               _sensorManager.setSensorName(nId, TKTOCHAR(tkArgs, 3));
            } else {
               println(F("usage: sensor name <id> <name>"));
            }
         } else if (strSubCmd == "get") {
            float f = _sensorManager.getSensorValueFloat(TKTOINT(tkArgs, 2, INVALID_FLOAT));
            if (!std::isnan(f)) {
               println(f);
            } else {
               println(F("invalid sensor id!"));
            }
         } else {
            if (__console.hasFS()) {
               __console.man(cmd.c_str());
            } else {
#ifndef MINIMAL_HELP
               println(F("sensor commands:"));
               println(F("  list"));
               println(F("  name <id> <name>"));
               println(F("  get <id>"));
#endif
            }
         }
      } else if (cmd == "relay") {
         String strName = TKTOCHAR(tkArgs, 1);
         String strSubCmd = TKTOCHAR(tkArgs, 2);
         
         CxDevice* pDev = _gpioDeviceManager.getDevice(strName.c_str());
         
         if (strName == "list") {
            _gpioDeviceManager.printList("relay");
         } else if (pDev) {
            String strType = pDev->getTypeSz();
            
            if (strType != "relay") {
               __console.println(F("device is not a relay!"));
            } else {
               CxRelay* p = static_cast<CxRelay*>(_gpioDeviceManager.getDevice(strName.c_str()));
               
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
   
   void gpioAction() {
      // check for gpio events
      _gpioDeviceManager.loop(__console.isAPMode());
   }

   void startWiFi(const char* ssid = nullptr, const char* pw = nullptr) {
      
#ifndef ESP_CONSOLE_NOWIFI
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
      
      char szSSID[20];
      char szPassword[25];
      char szHostname[80];
      
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
      
      print(ESC_CLEAR_LINE "\r");
      printf(F(ESC_ATTR_BOLD "WiFi: connecting to %s..." ESC_ATTR_RESET), szSSID);
      
      Led1.off();
      
      if (WiFi.status() != WL_CONNECTED) {
         println(F(ESC_ATTR_BOLD ESC_TEXT_BRIGHT_RED "not connected!" ESC_ATTR_RESET));
         __console.error("WiFi not connected.");
         Led1.blinkError();
      } else {
         println(F(ESC_TEXT_BRIGHT_GREEN "connected!" ESC_ATTR_RESET));
         _CONSOLE_INFO("WiFi connected.");
         Led1.flashOk();
#ifdef ESP32
         __console.setHostName(WiFi.getHostname().c_str());
#else
         __console.setHostName(WiFi.hostname().c_str());
#endif
      }
      
#endif /* Arduino */
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
   }


#endif /* ESP_CONSOLE_NOWIFI */

public:
   static void loadCap() {
      CAPREG(CxCapabilityExt);
      CAPLOAD(CxCapabilityExt);
   };
};



#endif /* CxCapabilityExt */

