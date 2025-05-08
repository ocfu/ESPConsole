/**
 * @file CxCapabilityMqttHA.hpp
 * @brief MQTT Home Assistant Capability for ESP-based projects
 *
 * This file defines the `CxCapabilityMqttHA` class, which provides MQTT Home Assistant
 * capabilities for an ESP-based project. It includes methods for setting up, managing,
 * and executing MQTT Home Assistant-related commands.
 *
 * @date created by ocfu on 09.01.25
 * @copyright © 2025 ocfu
 *
 * Key Features:
 * 1. Classes and Enumerations:
 *    - CxCapabilityMqttHA: Provides MQTT Home Assistant capabilities.
 *
 * 2. CxCapabilityMqttHA Class:
 *    - Manages MQTT Home Assistant properties and methods.
 *    - Provides methods to enable/disable Home Assistant, set/get sensor values, and update sensor readings.
 *    - Registers and unregisters sensors with the CxSensorManager.
 *
 * Relationships:
 * - CxCapabilityMqttHA is a subclass of CxCapability and interacts with CxSensorManager to register/unregister sensors.
 *
 * How They Work Together:
 * - CxCapabilityMqttHA represents MQTT Home Assistant capabilities with specific properties and methods.
 * - Sensors based on CxMqttHASensor register themselves with the CxSensorManager upon creation and unregister upon destruction.
 * - The CxCapabilityMqttHA can initialize, end, and print sensor information.
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
#ifndef CxCapabilityMqttHA_hpp
#define CxCapabilityMqttHA_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityMqtt.hpp"

#include "../tools/CxMqttHAManager.hpp"

/// defines for the index of the diagnostics objects
#define DIAG_STATUS 0
#define DIAG_RECONNECTS 1
#define DIAG_LINKQUALITY 2
#define DIAG_FREE_MEM 3
#define DIAG_LAST_RESTART 4
#define DIAG_UPTIME 5
#define DIAG_RESTART_REASON 6
#define DIAG_STACK 7
#define DIAG_STACK_LOW 8

#define DIAGNOTICS_COUNT 9

/**
 * @class CxCapabilityMqttHA
 * @brief Provides MQTT Home Assistant capabilities for an ESP-based project.
 * @details The `CxCapabilityMqttHA` class manages MQTT Home Assistant properties and methods.
 * It includes methods for enabling/disabling Home Assistant, setting/getting sensor values,
 * and updating sensor readings. The class registers and unregisters sensors with the CxSensorManager.
 */
class CxCapabilityMqttHA : public CxCapability {
private:
   /// access to the instances of the master console, MQTT HA device,  MQTT manager and sensor manager
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();
   CxMqttHADevice& _mqttHAdev = CxMqttHADevice::getInstance();
   CxMqttManager& __mqttManager = CxMqttManager::getInstance();
   CxSensorManager& _sensorManager = CxSensorManager::getInstance();
   CxGPIODeviceManagerManager& _gpioDeviceManager = CxGPIODeviceManagerManager::getInstance();


   bool _bHAEnabled = false;

   /// timer for updating sensor data
   CxTimer60s _timerUpdate;
   
   /// maps of Home Assistant diagnostics and sensors
   std::map<uint8_t, std::unique_ptr<CxMqttHADiagnostic>> _mapHADiag;
   std::vector<std::unique_ptr<CxMqttHASensor>> _vHASensor;
   
   std::vector<std::unique_ptr<CxMqttHAButton>> _vHAButton;
   std::vector<std::unique_ptr<CxMqttHASwitch>> _vHASwitch; // for relay devices
   

public:
   /// Default constructor and default capabilities methods.
   explicit CxCapabilityMqttHA()
   : CxCapability("mqttha", getCmds()) {}
   static constexpr const char* getName() { return "ha"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "ha" };
      return commands;
   }
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityMqttHA>();
   }
   
   /// Destructor to end the capability and clear the sensor objects.
   ~CxCapabilityMqttHA() {
      enableHA(false);
      _mapHADiag.clear();
      _vHASensor.clear();
      _vHAButton.clear();
      _vHASwitch.clear();
   }
   
   /// Setup method to initialize the capability and register
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*__console.getStream());
      __bLocked = false;
      
      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());
      
      
      // increase the PubSubClient buffer size as for HA the payload could be pretty long, especially for discovery topics.
      __mqttManager.setBufferSize(1024);
      
      /// load diagnostics items for the HA device for debugging purposes.
      _mapHADiag[DIAG_STATUS] = std::make_unique<CxMqttHADiagnostic>("Status", "diagstatus", true, __mqttManager.getRootPath());
      _mapHADiag[DIAG_RECONNECTS] = std::make_unique<CxMqttHADiagnostic>("Reconnects/h", "diagreconnects", nullptr, "1/h");
      _mapHADiag[DIAG_LINKQUALITY] = std::make_unique<CxMqttHADiagnostic>("Linkquality", "diaglink", nullptr, "rssi");
      _mapHADiag[DIAG_FREE_MEM] = std::make_unique<CxMqttHADiagnostic>("Free mem", "diagmem", nullptr, "bytes");
      _mapHADiag[DIAG_LAST_RESTART] = std::make_unique<CxMqttHADiagnostic>("Last Restart", "diagrestart", "timestamp", "");
      _mapHADiag[DIAG_UPTIME] = std::make_unique<CxMqttHADiagnostic>("Up Time", "diaguptime", "duration", "s");
      _mapHADiag[DIAG_RESTART_REASON] = std::make_unique<CxMqttHADiagnostic>("Restart Reason", "diagreason", nullptr, nullptr);
      _mapHADiag[DIAG_STACK] = std::make_unique<CxMqttHADiagnostic>("Stack", "diagstack", nullptr, "bytes");
      _mapHADiag[DIAG_STACK_LOW] = std::make_unique<CxMqttHADiagnostic>("Stack Low", "diagstacklow", nullptr, "bytes");

      __console.executeBatch("init", getName());

      
      /// enable MQTT HA
      if (isEnabled()) enableHA(true);

   }
   
   /// Loop method to update sensor data and diagnostics
   void loop() override {
      if (_timerUpdate.isDue()) {
         if (_mapHADiag[DIAG_STATUS]) {
            StaticJsonDocument<256> doc;
            doc[F("heartbeat")] = millis();
            doc[F("uptime")] = __console.getUpTimeISO();
            doc[F("connects")] = __mqttManager.getConnectCntr();
#ifdef ARDUINO
            doc[F("RSSI")] = WiFi.RSSI();
#endif
            _mapHADiag[DIAG_STATUS]->publishAttributes(doc);
         }
         
         if (_mapHADiag[DIAG_RECONNECTS]) {
            static uint32_t n1hBucket = 0;
            static uint32_t nLastCounter = 0;
            static unsigned long nTimer1h = 0;
            
            n1hBucket = (uint32_t)fmax(n1hBucket, __mqttManager.getConnectCntr() - nLastCounter);
            
            _mapHADiag[1]->publishAvailability(true);
            
            if ((millis() - nTimer1h) > (3600000)) {
               _mapHADiag[DIAG_RECONNECTS]->publishState(n1hBucket, 0);
               nTimer1h = millis();
               n1hBucket = 0;
               nLastCounter = __mqttManager.getConnectCntr();
            }
         }
#ifdef ARDUINO
         if (_mapHADiag[DIAG_LINKQUALITY]) _mapHADiag[DIAG_LINKQUALITY]->publishState(WiFi.RSSI(), 0);
         if (_mapHADiag[DIAG_FREE_MEM]) _mapHADiag[DIAG_FREE_MEM]->publishState(ESP.getFreeHeap(), 0);
#endif
         if (_mapHADiag[DIAG_UPTIME]) _mapHADiag[DIAG_UPTIME]->publishState(__console.getUpTimeSeconds(), 0);
         if (_mapHADiag[DIAG_STACK]) _mapHADiag[DIAG_STACK]->publishState(g_Stack.getSize(), 0);
         if (_mapHADiag[DIAG_STACK_LOW]) _mapHADiag[DIAG_STACK_LOW]->publishState(g_Stack.getLow(), 0);
         
      }
      /// update sensor data
      for (auto& pHASensor : _vHASensor) {
         if (pHASensor->isDue()) {
            pHASensor->publishState(pHASensor->getSensor()->getFloatValue(), 2);
         }
      }
   }
   
   bool execute(const char *szCmd, uint8_t nClient) override {
            
      // validate the call
      if (!szCmd) return false;
      
      // get the arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
      String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
            
      if (cmd == "?") {
         printCommands();
      }    if (cmd == "ha") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         String strSub2Cmd = TKTOCHAR(tkArgs, 2);
         String strEnv = ".ha";
         if (strSubCmd == "enable") {
            _bHAEnabled = (bool)TKTOINT(tkArgs, 2, 0);
            enableHA(_bHAEnabled);
         } else if (strSubCmd == "list") {
            _mqttHAdev.printList(getIoStream());
         } else if (strSubCmd == "sensor") {
            if (strSub2Cmd == "add") {
               addSensor(TKTOCHAR(tkArgs, 3), TKTOINT(tkArgs, 4, 60000));
            } else if (strSub2Cmd == "del") {
               deleteSensor(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "button") {
            if (strSub2Cmd == "add") {
               addButton(TKTOCHAR(tkArgs, 3));
            } else if (strSub2Cmd == "del") {
               deleteButton(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "switch") {
            if (strSub2Cmd == "add") {
               addSwitch(TKTOCHAR(tkArgs, 3), TKTOCHAR(tkArgs, 4), TKTOCHAR(tkArgs, 5));
            } else if (strSub2Cmd == "del") {
               deleteSwitch(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "state") {
            // ha state <name> <state>
            CxMqttHABase* pHAItem = _mqttHAdev.findItem(TKTOCHAR(tkArgs, 2));
            if (pHAItem) {
               pHAItem->publishState(TKTOCHAR(tkArgs, 3));
            }
         }
         else {
            printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), _bHAEnabled);
            __console.man(getName());
         }
      } else {
         return false;
      }
      g_Stack.update();
      return true;
   }
      
   bool isEnabled() {return _bHAEnabled;}
   void setEnabled(bool set) {_bHAEnabled = set;}
   
   /**
    * @brief Enables or disables the MQTT Home Assistant support.
    * @param enabled - true to enable, false to disable.
   */
   void enableHA(bool enabled) {
      if (__mqttManager.getName()[0]) {
         _mqttHAdev.setFriendlyName(__mqttManager.getName());
      } else {
         _mqttHAdev.setFriendlyName(__console.getAppName());
      }
      _mqttHAdev.setName(_mqttHAdev.getFriendlyName());
      _mqttHAdev.setModel(__console.getAppName());
      
      // the device defines the topic base by dedault
      // Note: all topics are relative to the root topic defined in mqtt
      _mqttHAdev.setTopicBase("ha");
      _mqttHAdev.setManufacturer("ocfu");
      _mqttHAdev.setSwVersion(__console.getAppVer());
      _mqttHAdev.setHwVersion(::getChipType());
      _mqttHAdev.setUrl(__console.getVariable("url"));
      _mqttHAdev.setStrId();
   
      _mqttHAdev.regItems(enabled);
      //_mqttHAdev.publishAvailability(enabled);
      _mqttHAdev.publishAvailabilityAllItems();
      
      if (enabled) {
         if (_mapHADiag[DIAG_RESTART_REASON]) _mapHADiag[DIAG_RESTART_REASON]->publishState(::getResetInfo());
         if (_mapHADiag[DIAG_LAST_RESTART]) _mapHADiag[DIAG_LAST_RESTART]->publishState(__console.getStartTime());
         
         /// make the timer active for an instant update in the loop()
         _timerUpdate.makeDue();
      } else {
         _timerUpdate.stop();
      }      
   }
   
   /// Adds a sensor to the MQTT Home Assistant device.
   /// _vHASensor is a vector of unique pointers to CxMqttHASensor objects
   void addSensor(const char* szName, uint32_t nPeriod = 60000) {
      CxSensor* pSensor = _sensorManager.getSensor(szName);
      if (pSensor) {
         if (!_mqttHAdev.findItem(szName)) _vHASensor.push_back(std::make_unique<CxMqttHASensor>(pSensor, nPeriod)); /// implicitly registers the new item in the device.
      } else {
         __console.printf(F("Sensor '%s' not found."), szName);
      }
   }
   
   /// Deletes a sensor from the MQTT Home Assistant device
   void deleteSensor(const char* szName) {
      for (auto it = _vHASensor.begin(); it != _vHASensor.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHASensor.erase(it);
            break;
         }
      }
   }
   
   void addButton(const char* szName) {
      CxButton* pDevice = static_cast<CxButton*>(_gpioDeviceManager.getDevice(szName, "button"));
      if (pDevice) {
         if (!_mqttHAdev.findItem(szName)) {
            _vHAButton.push_back(std::make_unique<CxMqttHAButton>(pDevice));
            pDevice->addCallback([this, pDevice](CxDevice* dev, uint8_t id, const char* cmd) {
               for (auto& pButton : _vHAButton) {
                  if (strcmp(pButton->getName(), pDevice->getName()) == 0) {
                     if (id == (uint8_t)CxButton::EBtnEvent::pressed) {
                        pButton->publishState("pressed");
                     } else if (id == CxButton::EBtnEvent::singlepress) {
                        pButton->publishState("single");
                     }
                     pButton->publishState("");
                     break;
                  }
               }
            });
         }
      } else {
         __console.printf(F("Button '%s' not found."), szName);
      }
   }
   
   void deleteButton(const char* szName) {
      for (auto it = _vHAButton.begin(); it != _vHAButton.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHAButton.erase(it);
            break;
         }
      }
   }
   
   void addSwitch(const char* szName, const char* fn = nullptr, const char* cmd = nullptr) {
      CxRelay* pRelay = static_cast<CxRelay*>(_gpioDeviceManager.getDevice(szName, "relay"));
      if (pRelay) {
         // set call back to publish the state of the relay on change
         pRelay->addCallback([this, pRelay](CxDevice* dev, uint8_t id, const char* cmd) {
            for (auto& pSwitch : _vHASwitch) {
               if (strcmp(pSwitch->getName(), pRelay->getName()) == 0) {
                  if (id == CxRelay::ERelayEvent::relayon) {
                     pSwitch->publishState(pRelay->isOn());
                  } else if (id == CxRelay::ERelayEvent::relayoff) {
                     pSwitch->publishState(pRelay->isOn());
                  }
                  return;
               }
            }
         });
         
         if (!_mqttHAdev.findItem(szName)) _vHASwitch.push_back(std::make_unique<CxMqttHASwitch>(pRelay, [this, pRelay](const char* topic, uint8_t* payload, unsigned int len) -> bool {
            // this callback handles commands on the subscribed topic (./cmd)
            for (auto& pSwitch : _vHASwitch) {
               if (strcmp(pSwitch->getName(), pRelay->getName()) == 0) {
                  if (strncmp((char*)payload, "ON", 2) == 0) {
                     pRelay->on();
                  } else if (strncmp((char*)payload, "OFF", 3) == 0) {
                     pRelay->off();
                  } else {
                     return false;
                  }
                  return true;
               }
            }
            return false;
         }));
      } else {
         CxGPIOVirtual* pVirtual = static_cast<CxGPIOVirtual*>(_gpioDeviceManager.getDevice(szName, "virtual"));
         if (pVirtual) {
            // set call back to publish the state of the relay on change
            pVirtual->addCallback([this, pVirtual](CxDevice* dev, uint8_t id, const char* cmd) {
               for (auto& pSwitch : _vHASwitch) {
                  if (strcmp(pSwitch->getName(), pVirtual->getName()) == 0) {
                     if (id == CxRelay::ERelayEvent::relayon) {
                        pSwitch->publishState(pVirtual->isOn());
                     } else if (id == CxRelay::ERelayEvent::relayoff) {
                        pSwitch->publishState(pVirtual->isOn());
                     }
                     return;
                  }
               }
            });
            
            if (!_mqttHAdev.findItem(szName)) {
               _vHASwitch.push_back(std::make_unique<CxMqttHASwitch>(pVirtual, [this, pVirtual](const char* topic, uint8_t* payload, unsigned int len) -> bool {
                  // this callback handles commands on the subscribed topic (./cmd)
                  for (auto& pSwitch : _vHASwitch) {
                     if (strcmp(pSwitch->getName(), pVirtual->getName()) == 0) {
                        if (strncmp((char*)payload, "ON", 2) == 0) {
                           pVirtual->on();
                        } else if (strncmp((char*)payload, "OFF", 3) == 0) {
                           pVirtual->off();
                        } else {
                           return false;
                        }
                        return true;
                     }
                  }
                  return false;
               }));
            }
         } else {
            __console.printf(F("Device '%s' is not a ok."), szName);
         }
      }
   }
   
   void deleteSwitch(const char* szName) {
      for (auto it = _vHASwitch.begin(); it != _vHASwitch.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHASwitch.erase(it);
            break;
         }
      }
   }

   static void loadCap() {
      CAPREG(CxCapabilityMqttHA);
      CAPLOAD(CxCapabilityMqttHA);
   };
};

#endif /* CxCapabilityMqttHA_hpp */
