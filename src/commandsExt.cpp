#include "commands.h"
#ifdef ESP_CONSOLE_EXT

#include <tools/CxGpioTracker.hpp>
#include <tools/CxLed.hpp>
#include <tools/CxButton.hpp>
#include <tools/CxRelay.hpp>
#include <tools/CxContact.hpp>
#include <tools/CxAnalog.hpp>
#include <esphw.h>
#include <tools/CxSensorManager.hpp>
#include <tools/espmath.h>

#include <ArduinoJson.h>

CxGPIOTracker& __gpioTracker = CxGPIOTracker::getInstance();
CxGPIODeviceManagerManager& __gpioDeviceManager = CxGPIODeviceManagerManager::getInstance();
CxSensorManager& __sensorManager = CxSensorManager::getInstance();

std::map<String, String> __mapProcessJsonDataItems;

CxLed Led1(LED_BUILTIN, "led1");

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

void ledAction() {
   Led1.action();
}

void gpioAction() {
   // check for gpio events
   bool bIsDegraded = false;
#ifdef ESP_CONSOLE_WIFI
   bIsDegraded = __console.isAPMode();
#endif
   __gpioDeviceManager.loop(bIsDegraded);
}

void setupExt() {
}

void loopExt() {
   /// update led indications, if any
   ledAction();

//   _sensorManager.update();

   /// check gpio events
   gpioAction();
}

// Command gpio
bool cmd_gpio(CxStrToken& tkArgs) {
   String strSubCmd = TKTOCHAR(tkArgs, 1);
   uint8_t nPin = TKTOINT(tkArgs, 2, INVALID_PIN);
   int16_t nValue = TKTOINT(tkArgs, 3, -1);
   String strValue = TKTOCHAR(tkArgs, 3);

   if (strSubCmd == "state") {
      CxTablePrinter table(getIoStream());
#ifndef MINIMAL_COMMAND_SET
      table.printHeader({F("Pin"), F("Mode"), F("inv"), F("State"), F("PWM"), F("Value")}, {3, 10, 3, 5, 8, 6});
#else
      table.printHeader({F("Pin"), F("Mode"), F("inv"), F("State")}, {3, 10, 3, 5});
#endif

      for (const auto& pin : __gpioTracker.getPins()) {
         if (nPin != INVALID_PIN && nPin != pin) continue;  // skip if pin does not match

         CxGPIO gpio(pin);
         gpio.get();
         if (gpio.isAnalog()) {
            if (nPin != INVALID_PIN) {
               __console.setOutputVariable(gpio.getAnalogValue());
            }
#ifndef MINIMAL_COMMAND_SET
            table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", "n/a", "n/a", gpio.isAnalog() ? String(gpio.getAnalogValue()) : ""});
#else
            table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", "n/a"});
#endif
         } else {
            if (nPin != INVALID_PIN) {
               __console.setOutputVariable(gpio.getDigitalState() ? "HIGH" : "LOW");
            }
#ifndef MINIMAL_COMMAND_SET
            table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", gpio.getDigitalState() ? "HIGH" : "LOW", gpio.isPWM() ? "Endabled" : "Disabled", gpio.isAnalog() ? String(gpio.getAnalogValue()) : ""});
#else
            table.printRow({String(pin).c_str(), gpio.getPinModeSz(), gpio.isInverted() ? "yes" : "no", gpio.getDigitalState() ? "HIGH" : "LOW"});
#endif
         }
      }
      return true;
   } else if (strSubCmd == "set") {
      if (__gpioTracker.isValidPin(nPin)) {
         CxGPIO gpio(nPin);
         if (nValue < 0) {  // setting the pin mode
            if (strValue == "in") {
               gpio.setPinMode(INPUT);
            } else if (strValue == "out") {
               gpio.setPinMode(OUTPUT);
            } else if (strValue == "pwm") {
               // todo
               __console.println(F("feature is not yet implemented!"));
            } else if (strValue == "inverted") {
               gpio.setInverted(true);
            } else if (strValue == "non-inverted") {
               gpio.setInverted(false);
            } else if (strValue == "analog") {
            } else if (strValue == "virtual") {
            } else {
               __console.printf(F("invalid pin mode!"));
               return false;
            }
         } else if (nValue < 1024) {
            CxGPIODevice* pDev = __gpioDeviceManager.getDeviceByPin(nPin);
            if (pDev) pDev->set(nValue);
         } else {
            __console.printf(F("invalid value!"));
            return false;
         }
      } else {
         __console.println("invalid");
         __gpioTracker.printInvalidReason(getIoStream(), nPin);
         return false;
      }
   } else if (strSubCmd == "get") {
      if (__gpioTracker.isValidPin(nPin)) {
         CxGPIO gpio(nPin);
         gpio.printState(getIoStream());
      } else {
         __gpioTracker.printInvalidReason(getIoStream(), nPin);
         return false;
      }
   } else if (strSubCmd == "list") {
      __gpioDeviceManager.printList();      
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
            CxButton* pButton = static_cast<CxButton*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pButton) {
               pButton->setName(strName.c_str());
               pButton->setInverted(bInverted);
               pButton->setCmd(strGpioCmd.c_str());
               pButton->begin();
            } else {
               if (strGpioCmd == "reset") {
                  CxButtonReset* p = new CxButtonReset(nPin, strName.c_str(), bInverted, bPullup);
                  if (p) {
                     p->begin();
                  }
               } else {
                  CxButton* p = new CxButton(nPin, strName.c_str(), bInverted, bPullup, strGpioCmd.c_str());
                  if (p) {
                     p->begin();
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
            } else {
               CxLed* p = static_cast<CxLed*>(__gpioDeviceManager.getDeviceByPin(nPin));
               if (p) {
                  p->setName(strName.c_str());
                  p->setInverted(bInverted);
                  p->setCmd(strGpioCmd.c_str());
                  p->begin();
                  p->off();
               } else {
                  CxLed* p = new CxLed(nPin, strName.c_str(), bInverted);
                  if (p) {
                     // p->setFriendlyName();
                     p->begin();
                  }
               }
            }
         } else if (strType == "relay") {
            /// TODO: consider dyanmic cast to ensure correct type
            CxRelay* pRelay = static_cast<CxRelay*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pRelay) {
               pRelay->setName(strName.c_str());
               pRelay->setInverted(bInverted);
               pRelay->setCmd(strGpioCmd.c_str());
               pRelay->begin();
            } else {
               CxRelay* p = new CxRelay(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
               if (p) {
                  p->begin();
               }
            }
         } else if (strType == "contact") {
            CxContact* pContact = static_cast<CxContact*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pContact) {
               pContact->setName(strName.c_str());
               pContact->setInverted(bInverted);
               pContact->setCmd(strGpioCmd.c_str());
               pContact->begin();
            } else {
               CxContact* p = new CxContact(nPin, strName.c_str(), bInverted, bPullup, strGpioCmd.c_str());
               if (p) {
                  p->begin();
               }
            }
         } else if (strType == "counter") {
            CxCounter* pCounter = static_cast<CxCounter*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pCounter) {
               pCounter->setName(strName.c_str());
               pCounter->setInverted(bInverted);
               pCounter->setCmd(strGpioCmd.c_str());
               pCounter->begin();
            } else {
               CxCounter* p = new CxCounter(nPin, strName.c_str(), bInverted, bPullup, strGpioCmd.c_str());
               if (p) {
                  p->begin();
               }
            }
         } else if (strType == "analog") {
            CxAnalog* pAnalog = static_cast<CxAnalog*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pAnalog) {
               pAnalog->setName(strName.c_str());
               pAnalog->setInverted(bInverted);
               pAnalog->setCmd(strGpioCmd.c_str());
               pAnalog->setTimer(TKTOINT(tkArgs, 7, 1000));  // default update rate 1s
               pAnalog->begin();
            } else {
               CxAnalog* p = new CxAnalog(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
               if (p) {
                  p->begin();
               }
            }
         } else if (strType == "virtual") {
            CxGPIOVirtual* pVirtual = static_cast<CxGPIOVirtual*>(__gpioDeviceManager.getDeviceByName(strName.c_str()));
            if (pVirtual) {
               pVirtual->setName(strName.c_str());
               pVirtual->setInverted(bInverted);
               pVirtual->setCmd(strGpioCmd.c_str());
               pVirtual->begin();
            } else {
               CxGPIOVirtual* p = new CxGPIOVirtual(nPin, strName.c_str(), bInverted, strGpioCmd.c_str());
               if (p) {
                  p->begin();
               }
            }
         } else {
            __console.println(F("invalid device type!"));
            return false;
         }
      } else {
         __console.println(F("invalid pin!"));
         return false;
      }
   } else if (strSubCmd == "del") {
      // FIXME: delete command crashes the system
      String strName = TKTOCHAR(tkArgs, 2);
      if (strName == "led1") {
         Led1.setPin(INVALID_PIN);
         Led1.setName("");
      } else {
         CxGPIODevice* p = __gpioDeviceManager.getDevice(strName.c_str());
         if (p) {
            delete p;
         } else {
            __console.println(F("device not found!"));
            return false;
         }
         __gpioDeviceManager.removeDevice(strName.c_str());
      }
   } else if (strSubCmd == "name") {
      if (__gpioTracker.isValidPin(nPin)) {
         CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);
         if (p) {
            p->setFriendlyName(strValue.c_str());
            p->setName(strValue.c_str());
         } else {
            __console.println(F("device not found!"));
            return false;
         }
      } else {
         __console.println(F("invalid pin!"));
         return false;
      }
   } else if (strSubCmd == "fn") {
      CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);

      if (p) {
         p->setFriendlyName(TKTOCHAR(tkArgs, 3));
      } else {
         __console.println(F("device not found!"));
         return false;
      }

   } else if (strSubCmd == "deb") {
      CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);

      if (p) {
         p->setDebounce(TKTOINT(tkArgs, 3, p->getDebounce()));
      } else {
         __console.println(F("device not found!"));
         return false;
      }
   } else if (strSubCmd == "isr") {
      // isr <pin> <id> [<debounce time>]
      CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);
      if (p) {
         p->setDebounce(TKTOINT(tkArgs, 4, p->getDebounce()));
         p->setISR(TKTOINT(tkArgs, 3, INVALID_UINT8));
         p->enableISR();
      } else {
         CxTablePrinter table(getIoStream());
         table.printHeader({F("ID"), F("Counter"), F("Debounce")}, {3, 10, 8});
         for (int i = 0; i < 3; i++) {
            table.printRow({String(i).c_str(), String(g_anEdgeCounter[i]).c_str(), String(g_anDebounceDelay[i]).c_str()});
         }
      }
   } else if (strSubCmd == "let" && tkArgs.count() > 4) {
      String strOperator = TKTOCHAR(tkArgs, 3);
      CxGPIODevice* dev1 = __gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 2));
      CxGPIODevice* dev2 = __gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 4));

      if (strOperator == "=") {
         if (dev1 && dev2) {
            dev1->set(dev2->get());
         } else if (dev1) {
            String strValue = TKTOCHAR(tkArgs, 4);
            uint32_t nValue = INVALID_UINT32;
            char* end = nullptr;

            if (strValue.startsWith("$")) {  // we need this? substitution done on higher level actually
               __console.substituteVariables(strValue);
            }

            nValue = (uint32_t)std::strtol(strValue.c_str(), &end, 0);  // return as uint32_t with auto base

            // Check if the conversion failed (no characters processed or out of range)
            if (!end || end == strValue.c_str() || *end != '\0') {
               __console.error(F("cannot assign the value %s to %s (not a number)"), strValue.c_str(), dev1->getName());
            } else {
               dev1->set((bool)nValue);  // MARK: currently only bool is supported
            }
         } else {
            __console.println(F("device not found!"));
            return false;
         }
      }
   } 
   return true;
}
void help_gpio() {
   __console.println(F("gpio <subcmd> [args]"));
   __console.println(F("  subcmd: state, set, get, list, add, del, name, fn, deb, isr, let"));
   __console.println(F("  state - print current state of all GPIO pins"));
   __console.println(F("  set <pin> <mode|value> - set pin mode or value"));
   __console.println(F("  get <pin> - get pin state"));
   __console.println(F("  list - list all GPIO devices"));
   __console.println(F("  add <pin> <type> [name] [inverted] [cmd] [pullup] - add a GPIO device"));
   __console.println(F("  del <name> - delete a GPIO device by name"));
   __console.println(F("  name <pin> <name> - set friendly name for the pin"));
   __console.println(F("  fn <pin> <friendly_name> - set friendly name for the pin (alias)"));
   __console.println(F("  deb <pin> <debounce_time> - set debounce time for the pin in ms (default is 100ms)"));
   __console.println(F("  isr <pin> <id> [debounce_time] - set ISR for the pin with id (0-2) and optional debounce time in ms (default is 100ms)"));
   __console.println(F("  let <var_name> = <value|device_name> - assign value to variable or device state to variable"));
}

bool cmd_led(CxStrToken& tkArgs) {
   String strSubCmd = TKTOCHAR(tkArgs, 1);
   uint8_t nIndexOffset = 0;

   // Default to Led1 if no specific LED is specified by its name (led <name> | <subcommand> ...)
   CxLed* led = &Led1;

   // Check if a specific LED is specified (by name)
   CxLed* p = static_cast<CxLed*>(__gpioDeviceManager.getDeviceByName(TKTOCHAR(tkArgs, 1)));
   if (p) {
      led = p;
      strSubCmd = TKTOCHAR(tkArgs, 2);
      nIndexOffset = 1;
   }

   strSubCmd.toLowerCase();

   if (strSubCmd == "on") {
      led->on();
   } else if (strSubCmd == "off") {
      led->off();
   } else if (strSubCmd == "blink") {
      String strPattern = TKTOCHAR(tkArgs, 2 + nIndexOffset);
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
      } else {
         led->setBlink(TKTOINT(tkArgs, 2 + nIndexOffset, 1000), TKTOINT(tkArgs, 3 + nIndexOffset, 128));
      }
   } else if (strSubCmd == "flash") {
      String strPattern = TKTOCHAR(tkArgs, 2 + nIndexOffset);
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
         led->setFlash(TKTOINT(tkArgs, 2 + nIndexOffset, 250), TKTOINT(tkArgs, 3 + nIndexOffset, 128), TKTOINT(tkArgs, 4 + nIndexOffset, 1));
      }
   } else if (strSubCmd == "invert") {
      if (tkArgs.count() > 2 + nIndexOffset) {
         led->setInverted(TKTOINT(tkArgs, 2 + nIndexOffset, false));
      } else {
         led->setInverted(!led->isInverted());
         led->toggle();
      }
   } else if (strSubCmd == "toggle") {
      led->toggle();
   } 
   return true;
}
void help_led() {
   __console.println(F("led <subcmd> [args]"));
   __console.println(F("led <name <subcmd> [args]"));
   __console.println(F("  subcmd: on, off, blink, flash, invert, toggle"));
   __console.println(F("  on - turn on the LED"));
   __console.println(F("  off - turn off the LED"));
   __console.println(F("  blink [pattern] - blink the LED with a pattern (ok, error, busy, flash, data, wait, connect) or custom blink rate in ms and brightness (default is 1000ms and 128)"));
   __console.println(F("  flash [pattern] - flash the LED with a pattern (ok, error, busy, flash, data, wait, connect) or custom flash rate in ms and brightness (default is 250ms and 128)"));
   __console.println(F("  invert [true|false] - invert the LED logic (default is false)"));
   __console.println(F("  toggle - toggle the LED state"));
}  

// Command sensor
bool cmd_sensor(CxStrToken& tkArgs) {
   String strSubCmd = TKTOCHAR(tkArgs, 1);
   if (strSubCmd == "list") {
      __sensorManager.printList();
   } else if (strSubCmd == "name") {
      uint8_t nId = TKTOINT(tkArgs, 2, INVALID_UINT8);
      if (nId != INVALID_UINT8) {
         __sensorManager.setSensorName(nId, TKTOCHAR(tkArgs, 3));
      } else {
         __console.println(F("usage: sensor name <id> <name>"));
         return false;
      }
   } else if (strSubCmd == "get") {
      float f = __sensorManager.getSensorValueFloat(TKTOFLOAT(tkArgs, 2, INVALID_FLOAT));
      if (!std::isnan(f)) {
         __console.println(f);
         __console.setOutputVariable(f);
      } else {
         __console.println(F("invalid sensor id!"));
      }
   } else if (strSubCmd == "add" && tkArgs.count() > 5) {
      // sensor add <name> <type> <unit> <variable>
      String strVar = TKTOCHAR(tkArgs, 5);
      CxSensor* pSensor = __sensorManager.getSensor(TKTOCHAR(tkArgs, 2));

      if (pSensor) {
      } else {
         String strType = TKTOCHAR(tkArgs, 3);
         pSensor = new CxSensorGeneric(TKTOCHAR(tkArgs, 2), ECSensorType::other, TKTOCHAR(tkArgs, 4), [strVar]() -> float {
            if (strVar.length() > 0) {
               float fValue = 0.0F;
               char* end = nullptr;
               const char* szValue = __console.getVariable(strVar.c_str());
               if (szValue) {
                  fValue = std::strtod(szValue, &end);  // return as uint32_t with auto base
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
         }
      }
   } else if (strSubCmd == "del") {
      __sensorManager.removeSensor(TKTOCHAR(tkArgs, 2));
   }
   return true;
}
void help_sensor() {
   __console.println(F("sensor <subcmd> [args]"));
   __console.println(F("  subcmd: list, name, get, add, del"));
   __console.println(F("  list - list all sensors"));
   __console.println(F("  name <id> <name> - set sensor name by id"));
   __console.println(F("  get <id> - get sensor value by id"));
   __console.println(F("  add <name> <type> <unit> <variable> - add a new sensor"));
   __console.println(F("  del <name> - delete a sensor by name"));
}

// Command relay
bool cmd_relay(CxStrToken& tkArgs) {
   String strName = TKTOCHAR(tkArgs, 1);
   String strSubCmd = TKTOCHAR(tkArgs, 2);
   strSubCmd.toLowerCase();

   CxGPIODevice* pDev = __gpioDeviceManager.getDevice(strName.c_str());

   if (strName == "list") {
      __gpioDeviceManager.printList("relay");
   } else if (pDev) {
      String strType = pDev->getTypeSz();

      if (strType != "relay") {
         __console.println(F("device is not a relay!"));
      } else {
         CxRelay* p = static_cast<CxRelay*>(__gpioDeviceManager.getDevice(strName.c_str()));

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
         } else {
            __console.println(F("invalid relay command"));
            return false;
         }
      }
   }
   return true;
}
void help_relay() {
   __console.println(F("relay <name> <subcmd> [args]"));
   __console.println(F("  subcmd: list, on, off, toggle, offtimer, default"));
   __console.println(F("  list - list all relays"));
   __console.println(F("  on - turn on the relay"));
   __console.println(F("  off - turn off the relay"));
   __console.println(F("  toggle - toggle the relay state"));
   __console.println(F("  offtimer <ms> - set off timer in milliseconds (0 to disable)"));
   __console.println(F("  default <ms> - set default on time in milliseconds (0 to disable)"));
}

// Command smooth
bool cmd_smooth(CxStrToken& tkArgs) {
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
      return false;  // invalid input
   } else {
      float fValue = smoothRobust(reference, value, maxDiff, threshold, minAlpha, maxAlpha);
      __console.setOutputVariable(fValue);
      return (std::isnan(fValue) ? false : true);  // consider as non success, if nan was returned.
   }
   return true;
}

// Command max
bool cmd_max(CxStrToken& tkArgs) {
   // max <value1> <value2> [<value3> ...]
   // returns the maximum value of the given values
   // sets the output variable $> to the maximum value
   // sets the exit value to 0, if valid

   float fMax = TKTOFLOAT(tkArgs, 1, INVALID_FLOAT);
   if (std::isnan(fMax)) {
      return false;  // invalid input
   }

   for (size_t i = 2; i < tkArgs.count(); i++) {
      float fValue = TKTOFLOAT(tkArgs, i, INVALID_FLOAT);
      if (!std::isnan(fValue)) {
         fMax = std::max(fMax, fValue);
      }
   }

   __console.setOutputVariable(fMax);
   return true;
}

// Command min
bool cmd_min(CxStrToken& tkArgs) {
   // min <value1> <value2> [<value3> ...]
   // returns the minimum value of the given values
   // sets the output variable $> to the minimum value
   // sets the exit value to 0, if valid

   float fMin = TKTOFLOAT(tkArgs, 1, INVALID_FLOAT);
   if (std::isnan(fMin)) {
      return false;  // invalid input
   }
   for (size_t i = 2; i < tkArgs.count(); i++) {
      float fValue = TKTOFLOAT(tkArgs, i, INVALID_FLOAT);
      if (!std::isnan(fValue)) {
         fMin = std::min(fMin, fValue);
      }
   }
   __console.setOutputVariable(fMin);
   return true;
}

// Command processdata
bool cmd_processdata(CxStrToken& tkArgs) {
   String strType = TKTOCHAR(tkArgs, 1);
   if (strType == "json" && tkArgs.count() > 3) {
      if (!__mapProcessJsonDataItems.size()) {
         // register processdata method with the first set data item
         __console.setFuncProcessData([](const char* data) -> bool {
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
                  for (const auto& pair : __mapProcessJsonDataItems) {
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
            return bSucccess;
         });
      }
      __mapProcessJsonDataItems[TKTOCHAR(tkArgs, 2)] = TKTOCHAR(tkArgs, 3);
   } else if (strType == "list") {
      CxTablePrinter table(getIoStream());
      table.printHeader({F("Json Path"), F("Command")}, {20, 40});
      for (const auto& pair : __mapProcessJsonDataItems) {
         table.printRow({pair.first, pair.second.c_str()});
      }
   }
   return true;
}

// Command table in PROGMEM
const CommandEntry commandsExt[] PROGMEM = {
   {"gpio", cmd_gpio, help_gpio},
   {"led", cmd_led, help_led},
   {"sensor", cmd_sensor, help_sensor},
   {"relay", cmd_relay, help_relay},
   {"smooth", cmd_smooth, nullptr},
   {"max", cmd_max, nullptr},
   {"min", cmd_min, nullptr},
   {"processdata", cmd_processdata, nullptr},
    // Add more extended commands here
};



const size_t NUM_COMMANDS_EXT = sizeof(commandsExt) / sizeof(commandsExt[0]);

#endif