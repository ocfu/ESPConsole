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

#include "tools/CxMqttHAManager.hpp"

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

   /// maps of Home Assistant diagnostics and sensors
   std::vector<std::unique_ptr<CxMqttHASensor>> _vHASensor;
   
   std::vector<std::unique_ptr<CxMqttHAButton>> _vHAButton;
   std::vector<std::unique_ptr<CxMqttHASwitch>> _vHASwitch; // for relay devices
   std::vector<std::unique_ptr<CxMqttHASelect>> _vHASelect; // for select control
   std::vector<std::unique_ptr<CxMqttHANumber>> _vHANumber; // for number input
   std::vector<std::unique_ptr<CxMqttHAText>>   _vHAText;   // for text input
   std::vector<std::unique_ptr<CxMqttHADiagnostic>>   _vHADiag;   // for diagnostics


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
      
      __console.executeBatch("init", getName());
      
      /// enable MQTT HA
      if (isEnabled()) enableHA(true);

   }
   
   /// Loop method to update sensor data and diagnostics
   void loop() override {
      /// update sensor data
      for (auto& pHASensor : _vHASensor) {
         if (pHASensor->isDue()) {
            pHASensor->publishState(pHASensor->getSensor()->getFloatValue(), 2);
         }
      }
   }
   
   uint8_t execute(const char *szCmd, uint8_t nClient) override {
            
      // validate the call
      if (!szCmd) return EXIT_FAILURE;
      
      // get the arguments into the token buffer
      CxStrToken tkArgs(szCmd, " ");
      
      // we have a command, find the action to take
      String cmd = TKTOCHAR(tkArgs, 0);
      
      // removes heading and trailing white spaces
      cmd.trim();
      
      uint8_t nExitValue = EXIT_FAILURE;

      if (cmd == "?") {
         nExitValue = printCommands();
      }    if (cmd == "ha") {
         String strSubCmd = TKTOCHAR(tkArgs, 1);
         String strSub2Cmd = TKTOCHAR(tkArgs, 2);
         String strEnv = ".ha";
         nExitValue = EXIT_SUCCESS;
         if (strSubCmd == "enable") {
            _bHAEnabled = (bool)TKTOINT(tkArgs, 2, 0);
            nExitValue = enableHA(_bHAEnabled);
         } else if (strSubCmd == "list") {
            _mqttHAdev.printList(getIoStream());
         } else if (strSubCmd == "sensor") {
            if (strSub2Cmd == "add") {
               nExitValue = addSensor(TKTOCHAR(tkArgs, 3), TKTOINT(tkArgs, 4, 60000));
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteSensor(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "button") {
            if (strSub2Cmd == "add") {
               nExitValue = addButton(TKTOCHAR(tkArgs, 3));
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteButton(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "switch") {
            if (strSub2Cmd == "add") {
               nExitValue = addSwitch(TKTOCHAR(tkArgs, 3), TKTOCHAR(tkArgs, 4), TKTOCHAR(tkArgs, 5));
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteSwitch(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "select") {
            if (strSub2Cmd == "add") {
               nExitValue = addSelect(TKTOCHAR(tkArgs, 3), TKTOCHAR(tkArgs, 4), TKTOINT(tkArgs, 5, 0), {});
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteSelect(TKTOCHAR(tkArgs, 3));
            } else if (strSub2Cmd == "addopt") {
               nExitValue = addOptSelect(TKTOCHAR(tkArgs, 3), TKTOCHARAFTER(tkArgs, 4));
            }
         } else if (strSubCmd == "number") {
            if (strSub2Cmd == "add") {
               nExitValue = addNumber(TKTOCHAR(tkArgs, 3), TKTOCHAR(tkArgs, 4), TKTOINT(tkArgs, 5, 0), TKTOCHAR(tkArgs, 6));
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteNumber(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "text") {
            if (strSub2Cmd == "add") {
               nExitValue = addText(TKTOCHAR(tkArgs, 3), TKTOCHAR(tkArgs, 4), TKTOINT(tkArgs, 5, 0), TKTOCHAR(tkArgs, 6));
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteText(TKTOCHAR(tkArgs, 3));
            }
         } else if (strSubCmd == "diag") {
            // ha diag add <name> <fn> <variable> [<param>]
            if (strSub2Cmd == "add") {
               nExitValue = addDiag(TKTOCHAR(tkArgs, 3), TKTOCHAR(tkArgs, 4), TKTOCHAR(tkArgs, 5), TKTOCHARAFTER(tkArgs, 6));
            } else if (strSub2Cmd == "del") {
               nExitValue = deleteDiag(TKTOCHAR(tkArgs, 3));
            } else if (strSub2Cmd == "update") {
               /// update diag data
               for (auto& pHADiag : _vHADiag) {
                  const char* szValue = __console.getVariable(pHADiag->getVariable());
                  if (szValue) {
                     pHADiag->publishState(szValue);
                     DynamicJsonDocument doc(256);
                     doc[F("variable")] = pHADiag->getVariable();
                     pHADiag->publishAttributes(doc);
                  } else {
                     pHADiag->publishAvailability(false);
                  }
               }
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
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }
      
   bool isEnabled() {return _bHAEnabled;}
   void setEnabled(bool set) {_bHAEnabled = set;}
   
   /**
    * @brief Enables or disables the MQTT Home Assistant support.
    * @param enabled - true to enable, false to disable.
   */
   uint8_t enableHA(bool enabled) {
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
      _mqttHAdev.setUrl(__console.getVariable("URL"));
      _mqttHAdev.setStrId();
   
      _mqttHAdev.regItems(enabled);
      //_mqttHAdev.publishAvailability(enabled);
      _mqttHAdev.publishAvailabilityAllItems();
      
      String strCmd;
      strCmd.reserve(40);
      strCmd = "exec $(userscript) haenable ";
      strCmd += enabled ? 1 : 0;
      __console.processCmd(strCmd.c_str());
      return EXIT_SUCCESS;
   }
   
   /// Adds a sensor to the MQTT Home Assistant device.
   /// _vHASensor is a vector of unique pointers to CxMqttHASensor objects
   uint8_t addSensor(const char* szName, uint32_t nPeriod = 60000) {
      CxSensor* pSensor = _sensorManager.getSensor(szName);
      if (pSensor) {
         if (!_mqttHAdev.findItem(szName)) _vHASensor.push_back(std::make_unique<CxMqttHASensor>(pSensor, nPeriod)); /// implicitly registers the new item in the device.
      } else {
         __console.printf(F("Sensor '%s' not found."), szName);
         return EXIT_FAILURE;
      }
      return EXIT_SUCCESS;
   }
   
   /// Deletes a sensor from the MQTT Home Assistant device
   uint8_t deleteSensor(const char* szName) {
      for (auto it = _vHASensor.begin(); it != _vHASensor.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHASensor.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t addButton(const char* szName) {
      CxButton* pDevice = static_cast<CxButton*>(_gpioDeviceManager.getDevice(szName, "button"));
      if (pDevice) {
         if (!_mqttHAdev.findItem(szName)) {
            _vHAButton.push_back(std::make_unique<CxMqttHAButton>(pDevice));
            pDevice->addCallback([this, pDevice](CxGPIODevice* dev, uint8_t id, const char* cmd) {
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
            return EXIT_SUCCESS;
         }
      } else {
         __console.printf(F("Button '%s' not found."), szName);
      }
      return EXIT_FAILURE;
   }
   
   uint8_t deleteButton(const char* szName) {
      for (auto it = _vHAButton.begin(); it != _vHAButton.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHAButton.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t addSwitch(const char* szName, const char* fn = nullptr, const char* cmd = nullptr) {
      CxRelay* pRelay = static_cast<CxRelay*>(_gpioDeviceManager.getDevice(szName, "relay"));
      if (pRelay) {
         // set call back to publish the state of the relay on change
         pRelay->addCallback([this, pRelay](CxGPIODevice* dev, uint8_t id, const char* cmd) {
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
         return EXIT_SUCCESS;
      } else {
         CxGPIOVirtual* pVirtual = static_cast<CxGPIOVirtual*>(_gpioDeviceManager.getDevice(szName, "virtual"));
         if (pVirtual) {
            // set call back to publish the state of the relay on change
            pVirtual->addCallback([this, pVirtual](CxGPIODevice* dev, uint8_t id, const char* cmd) {
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
               return EXIT_SUCCESS;
            }
         } else {
            __console.printf(F("Device '%s' is not a ok."), szName);
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t deleteSwitch(const char* szName) {
      for (auto it = _vHASwitch.begin(); it != _vHASwitch.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHASwitch.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }
      
   uint8_t addSelect(const char* szName, const char* fn, bool bAsConfig, const std::vector<String>& vOpt) {
      if (!_mqttHAdev.findItem(szName)) {
         auto ptr = std::make_unique<CxMqttHASelect>(szName, vOpt, nullptr);
         CxMqttHASelect* rawPtr = ptr.get();
         
         if (ptr) {
            ptr->setFriendlyName(fn);
            if (bAsConfig) ptr->asConfig(); // this sets the retain flag on the /cmd topic and ensures to be set, afer restart
            
            // set the command callback for the topic subscription
            ptr->setCmdCb([this, rawPtr](const char* topic, uint8_t* payload, unsigned int len) -> bool {
               // callback
               CxMqttHASelect* me = static_cast<CxMqttHASelect*>(_mqttHAdev.findItem(rawPtr->getName()));
               if (me) {
                  String strCmd;
                  uint8_t nOpt = me->getOption(payload, len); // 1-based
                  strCmd.reserve(80);
                  strCmd = "exec $(userscript) ";
                  strCmd += rawPtr->getName();
                  strCmd += " ";
                  strCmd += nOpt;  // arg 1 is index number
                  strCmd += " ";
                  strCmd += me->getOptionStr(nOpt); // arg 2 is the text string of the option
                  strCmd += " TTT";
                  __console.processCmd(strCmd.c_str());
                  me->publishState(me->getOptionStr(nOpt));
               }
               return true;
            });
            _vHASelect.push_back(std::move(ptr));
            return EXIT_SUCCESS;
         } else {
            __console.error(F("error adding HA item (oom?)"));
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t addOptSelect(const char* szName, const char* szOpt) {
      if (!szName || !*szName || !szOpt) return EXIT_FAILURE;
      for (auto it = _vHASelect.begin(); it != _vHASelect.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->addOption(szOpt);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t deleteSelect(const char* szName) {
      for (auto it = _vHASelect.begin(); it != _vHASelect.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHASelect.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }

   uint8_t addNumber(const char* szName, const char* fn, bool bAsConfig, const char* szParam) {
      if (!_mqttHAdev.findItem(szName)) {
         auto ptr = std::make_unique<CxMqttHANumber>(szName);
         CxMqttHANumber* rawPtr = ptr.get();
         
         if (ptr) {
            CxStrToken tkParam(szParam, ",");
            ptr->setMin(TKTOINT(tkParam, 0, 0));
            ptr->setMax(TKTOINT(tkParam, 1, 100));
            ptr->setStep(TKTOINT(tkParam, 2, 10));
            ptr->setUnit(TKTOCHAR(tkParam, 3));

            ptr->setFriendlyName(fn);
            if (bAsConfig) ptr->asConfig(); // this sets the retain flag on the /cmd topic and ensures to be set, afer restart
            
            // set the command callback for the topic subscription
            ptr->setCmdCb([this, rawPtr](const char* topic, uint8_t* payload, unsigned int len) -> bool {
               // callback
               CxMqttHASelect* me = static_cast<CxMqttHASelect*>(_mqttHAdev.findItem(rawPtr->getName()));
               if (me) {
                  int32_t nValue = (int32_t)strtod((char*)payload, nullptr);
                  String strCmd;
                  strCmd.reserve(40);
                  strCmd = "exec $(userscript) ";
                  strCmd += rawPtr->getName();
                  strCmd += " ";
                  strCmd += nValue;
                  __console.processCmd(strCmd.c_str());
                  me->publishState(nValue, 0);
               }
               return true;
            });
            _vHANumber.push_back(std::move(ptr));
            return EXIT_SUCCESS;
         } else {
            __console.error(F("error adding HA item (oom?)"));
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t deleteNumber(const char* szName) {
      for (auto it = _vHANumber.begin(); it != _vHANumber.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHANumber.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }

   uint8_t addText(const char* szName, const char* fn, bool bAsConfig, const char* szParam) {
      if (!_mqttHAdev.findItem(szName)) {
         auto ptr = std::make_unique<CxMqttHAText>(szName);
         CxMqttHAText* rawPtr = ptr.get();
         
         if (ptr) {
            CxStrToken tkParam(szParam, ",");
            ptr->setMax(TKTOINT(tkParam, 0, 64));
            ptr->setFriendlyName(fn);
            if (bAsConfig) ptr->asConfig(); // this sets the retain flag on the /cmd topic and ensures to be set, afer restart
            
            // set the command callback for the topic subscription
            ptr->setCmdCb([this, rawPtr](const char* topic, uint8_t* payload, unsigned int len) -> bool {
               // callback
               CxMqttHASelect* me = static_cast<CxMqttHASelect*>(_mqttHAdev.findItem(rawPtr->getName()));
               if (me) {
                  String strCmd;
                  strCmd.reserve(40);
                  strCmd = "exec $(userscript) ";
                  strCmd += rawPtr->getName();
                  strCmd += " ";
                  strCmd += (char*) payload;
                  __console.processCmd(strCmd.c_str());
                  me->publishState((char*) payload);
               }
               return true;
            });
            _vHAText.push_back(std::move(ptr));
            return EXIT_SUCCESS;
         } else {
            __console.error(F("error adding HA item (oom?)"));
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t deleteText(const char* szName) {
      for (auto it = _vHAText.begin(); it != _vHAText.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHAText.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }

   uint8_t addDiag(const char* szName, const char* fn, const char* szVar, const char* szParam) {
      // szParam: <unit>   # more to be defined, as needed.
      if (!_mqttHAdev.findItem(szName)) {
         auto ptr = std::make_unique<CxMqttHADiagnostic>(szName);
         
         if (ptr) {
            CxStrToken tkParam(szParam, ",");
            ptr->setDClass(TKTOCHAR(tkParam, 0));
            ptr->setUnit(TKTOCHAR(tkParam, 1));
            ptr->setVariable(szVar);
            ptr->setFriendlyName(fn);
            _vHADiag.push_back(std::move(ptr));
            return EXIT_SUCCESS;
         } else {
            __console.error(F("error adding HA item (oom?)"));
         }
      }
      return EXIT_FAILURE;
   }
   
   uint8_t deleteDiag(const char* szName) {
      for (auto it = _vHADiag.begin(); it != _vHADiag.end(); ++it) {
         if (strcmp((*it)->getName(), szName) == 0) {
            (*it)->publishAvailability(false);
            _vHADiag.erase(it);
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }


   static void loadCap() {
      CAPREG(CxCapabilityMqttHA);
      CAPLOAD(CxCapabilityMqttHA);
   };
};

#endif /* CxCapabilityMqttHA_hpp */
