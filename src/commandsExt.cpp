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

   __sensorManager.update();

   /// check gpio events
   gpioAction();
}

// Command gpio
void cmd_gpio(CxStrToken& tkArgs) {
   uint8_t nPin = (TKTOINT(tkArgs, 2, INVALID_PIN) < INVALID_PIN) ? TKTOINT(tkArgs, 2, INVALID_PIN) : INVALID_PIN;
   int16_t nValue = TKTOINT(tkArgs, 3, -1);

   if (tkArgs.indexOf("state") == 1) {
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
   } else if (tkArgs.indexOf("set") == 1) {
      if (__gpioTracker.isValidPin(nPin)) {
         CxGPIO gpio(nPin);
         if (nValue < 0) {  // setting the pin mode
            if (tkArgs.indexOf("in") == 3) {
               gpio.setPinMode(INPUT);
            } else if (tkArgs.indexOf("out") == 3) {
               gpio.setPinMode(OUTPUT);
            } else if (tkArgs.indexOf("pwm") == 3) {
               // todo
               __console.println(F("feature is not yet implemented!"));
            } else if (tkArgs.indexOf("inverted") == 3) {
               gpio.setInverted(true);
            } else if (tkArgs.indexOf("non-inverted") == 3) {
               gpio.setInverted(false);
            } else if (tkArgs.indexOf("analog") == 3) {
            } else if (tkArgs.indexOf("virtual") == 3) {
            } else {
               __console.printf(F("invalid pin mode!"));
               __console.setExitValue(EXIT_FAILURE);
            }
         } else if (nValue < 1024) {
            CxGPIODevice* pDev = __gpioDeviceManager.getDeviceByPin(nPin);
            if (pDev) pDev->set(nValue);
         } else {
            __console.printf(F("invalid value!"));
            __console.setExitValue(EXIT_FAILURE);
         }
      } else {
         __console.println("invalid");
         __gpioTracker.printInvalidReason(getIoStream(), nPin);
         __console.setExitValue(EXIT_FAILURE);
      }
   } else if (tkArgs.indexOf("get") == 1) {
      if (__gpioTracker.isValidPin(nPin)) {
         CxGPIO gpio(nPin);
         gpio.printState(getIoStream());
      } else {
         __gpioTracker.printInvalidReason(getIoStream(), nPin);
         __console.setExitValue(EXIT_FAILURE);
      }
   } else if (tkArgs.indexOf("list") == 1) {
      __gpioDeviceManager.printList();
   } else if (tkArgs.indexOf("add") == 1) {
      if (nPin != INVALID_PIN) {
         const char* szName = tkArgs.count() > 4 ? TKTOCHAR(tkArgs, 4) : "";
         const char* szGpioCmd = tkArgs.count() > 6 ? TKTOCHAR(tkArgs, 6) : "";
         bool bInverted = TKTOINT(tkArgs, 5, false);
         bool bPullup = TKTOINT(tkArgs, 7, false);
         if (tkArgs.indexOf("button") == 3) {
            // FIXME: pointer without proper deletion? even if managed internally? maybe container as for the bme?
            /// TODO: consider dyanmic cast to ensure correct type
            CxButton* pButton = static_cast<CxButton*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pButton) {
               pButton->setName(szName);
               pButton->setInverted(bInverted);
               pButton->setCmd(szGpioCmd);
               pButton->begin();
            } else {
               if (tkArgs.indexOf("reset") == 6) {
                  CxButtonReset* p = new CxButtonReset(nPin, szName, bInverted, bPullup);
                  if (p) {
                     p->begin();
                  }
               } else {
                  CxButton* p = new CxButton(nPin, szName, bInverted, bPullup, szGpioCmd);
                  if (p) {
                     p->begin();
                  }
               }
            }
         } else if (tkArgs.indexOf("led") == 3) {
            if (tkArgs.indexOf("led1") == 4) {
               Led1.setPin(nPin);
               Led1.setPinMode(OUTPUT);
               Led1.setName(szName);
               Led1.setInverted(bInverted);
               Led1.setCmd(szGpioCmd);
               Led1.off();
            } else {
               CxLed* p = static_cast<CxLed*>(__gpioDeviceManager.getDeviceByPin(nPin));
               if (p) {
                  p->setName(szName);
                  p->setInverted(bInverted);
                  p->setCmd(szGpioCmd);
                  p->begin();
                  p->off();
               } else {
                  CxLed* p = new CxLed(nPin, szName, bInverted);
                  if (p) {
                     // p->setFriendlyName();
                     p->begin();
                  }
               }
            }
         } else if (tkArgs.indexOf("relay") == 3) {
            /// TODO: consider dyanmic cast to ensure correct type
            CxRelay* pRelay = static_cast<CxRelay*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pRelay) {
               pRelay->setName(szName);
               pRelay->setInverted(bInverted);
               pRelay->setCmd(szGpioCmd);
               pRelay->begin();
            } else {
               CxRelay* p = new CxRelay(nPin, szName, bInverted, szGpioCmd);
               if (p) {
                  p->begin();
               }
            }
         } else if (tkArgs.indexOf("contact") == 3) {
            CxContact* pContact = static_cast<CxContact*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pContact) {
               pContact->setName(szName);
               pContact->setInverted(bInverted);
               pContact->setCmd(szGpioCmd);
               pContact->begin();
            } else {
               CxContact* p = new CxContact(nPin, szName, bInverted, bPullup, szGpioCmd);
               if (p) {
                  p->begin();
               }
            }
         } else if (tkArgs.indexOf("counter") == 3) {
            CxCounter* pCounter = static_cast<CxCounter*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pCounter) {
               pCounter->setName(szName);
               pCounter->setInverted(bInverted);
               pCounter->setCmd(szGpioCmd);
               pCounter->begin();
            } else {
               CxCounter* p = new CxCounter(nPin, szName, bInverted, bPullup, szGpioCmd);
               if (p) {
                  p->begin();
               }
            }
         } else if (tkArgs.indexOf("analog") == 3) {
            CxAnalog* pAnalog = static_cast<CxAnalog*>(__gpioDeviceManager.getDeviceByPin(nPin));
            if (pAnalog) {
               pAnalog->setName(szName);
               pAnalog->setInverted(bInverted);
               pAnalog->setCmd(szGpioCmd);
               pAnalog->setTimer(TKTOINT(tkArgs, 7, 1000));  // default update rate 1s
               pAnalog->begin();
            } else {
               CxAnalog* p = new CxAnalog(nPin, szName, bInverted, szGpioCmd);
               if (p) {
                  p->begin();
               }
            }
         } else if (tkArgs.indexOf("virtual") == 3) {
            CxGPIOVirtual* pVirtual = static_cast<CxGPIOVirtual*>(__gpioDeviceManager.getDeviceByName(szName));
            if (pVirtual) {
               pVirtual->setName(szName);
               pVirtual->setInverted(bInverted);
               pVirtual->setCmd(szGpioCmd);
               pVirtual->begin();
            } else {
               CxGPIOVirtual* p = new CxGPIOVirtual(nPin, szName, bInverted, szGpioCmd);
               if (p) {
                  p->begin();
               }
            }
         } else {
            __console.println(F("invalid device type!"));
            __console.setExitValue(EXIT_FAILURE);
         }
      } else {
         __console.println(F("invalid pin!"));
         __console.setExitValue(EXIT_FAILURE);
      }
   } else if (tkArgs.indexOf("del") == 1) {
      // FIXME: delete command crashes the system
      const char* strName = TKTOCHAR(tkArgs, 2) ? TKTOCHAR(tkArgs, 2) : "";
      if (tkArgs.indexOf("led1") == 2) {
         Led1.setPin(INVALID_PIN);
         Led1.setName("");
      } else {
         CxGPIODevice* p = __gpioDeviceManager.getDevice(strName);
         if (p) {
            delete p;
         } else {
            __console.println(F("device not found!"));
            __console.setExitValue(EXIT_FAILURE);
         }
         __gpioDeviceManager.removeDevice(strName);
      }
   } else if (tkArgs.indexOf("name") == 1) {
      if (__gpioTracker.isValidPin(nPin)) {
         CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);
         if (p) {
            p->setFriendlyName(tkArgs.count() > 3 ? TKTOCHAR(tkArgs, 3) : "");
            p->setName(tkArgs.count() > 3 ? TKTOCHAR(tkArgs, 3) : "");
         } else {
            __console.println(F("device not found!"));
            __console.setExitValue(EXIT_FAILURE);
         }
      } else {
         __console.println(F("invalid pin!"));
         __console.setExitValue(EXIT_FAILURE);
      }
   } else if (tkArgs.indexOf("fn") == 1) {
      CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);

      if (p) {
         p->setFriendlyName(TKTOCHAR(tkArgs, 3));
      } else {
         __console.println(F("device not found!"));
         __console.setExitValue(EXIT_FAILURE);
      }

   } else if (tkArgs.indexOf("deb") == 1) {
      CxGPIODevice* p = __gpioDeviceManager.getDeviceByPin(nPin);

      if (p) {
         p->setDebounce(TKTOINT(tkArgs, 3, p->getDebounce()));
      } else {
         __console.println(F("device not found!"));
         __console.setExitValue(EXIT_FAILURE);
      }
   } else if (tkArgs.indexOf("isr") == 1) {
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
   } else if (tkArgs.indexOf("let") == 1 && tkArgs.count() > 4) {
      CxGPIODevice* dev1 = __gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 2));
      CxGPIODevice* dev2 = __gpioDeviceManager.getDevice(TKTOCHAR(tkArgs, 4));

      if (tkArgs.indexOf("=") == 3) {
         if (dev1 && dev2) {
            dev1->set(dev2->get());
         } else if (dev1) {
            const char* szValue = TKTOCHAR(tkArgs, 4);
            uint32_t nValue = INVALID_UINT32;
            char* end = nullptr;

            nValue = (uint32_t)std::strtol(szValue, &end, 0);  // return as uint32_t with auto base

            // Check if the conversion failed (no characters processed or out of range)
            if (!end || end == szValue || *end != '\0') {
               __console.error(F("cannot assign the value %s to %s (not a number)"), szValue, dev1->getName());
            } else {
               dev1->set((bool)nValue);  // MARK: currently only bool is supported
            }
         } else {
            __console.println(F("device not found!"));
            __console.setExitValue(EXIT_FAILURE);
         }
      }
   }
   __console.setExitValue(EXIT_FAILURE);
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
   __console.println(F("  let <gpio name> = <value|gpio_name|var name> - assign value to variable or device state to variable"));
}

void cmd_led(CxStrToken& tkArgs) {
   uint8_t nCmdIndex = 1;

   // Default to Led1 if no specific LED is specified by its name (led <name> | <subcommand> ...)
   CxLed* led = &Led1;

   // Check if a specific LED is specified (by name)
   CxLed* p = static_cast<CxLed*>(__gpioDeviceManager.getDeviceByName(TKTOCHAR(tkArgs, 1)));
   if (p) {
      led = p;
      nCmdIndex = 2;
   }

   if (tkArgs.indexOf("on") == nCmdIndex) {
      led->on();
   } else if (tkArgs.indexOf("off") == nCmdIndex) {
      led->off();
   } else if (tkArgs.indexOf("blink") == nCmdIndex) {
      if (tkArgs.indexOf("ok") == nCmdIndex + 1) {
         led->blinkOk();
      } else if (tkArgs.indexOf("error") == nCmdIndex + 1) {
         led->blinkError();
      } else if (tkArgs.indexOf("busy") == nCmdIndex + 1) {
         led->blinkBusy();
      } else if (tkArgs.indexOf("flash") == nCmdIndex + 1) {
         led->blinkFlash();
      } else if (tkArgs.indexOf("data") == nCmdIndex + 1) {
         led->blinkData();
      } else if (tkArgs.indexOf("wait") == nCmdIndex + 1) {
         led->blinkWait();
      } else if (tkArgs.indexOf("connect") == nCmdIndex + 1) {
         led->blinkConnect();
      } else {
         led->setBlink(TKTOINT(tkArgs, nCmdIndex + 1, 1000), TKTOINT(tkArgs, nCmdIndex + 2, 128));
      }
   } else if (tkArgs.indexOf("flash") == nCmdIndex) {
      if (tkArgs.indexOf("ok") == nCmdIndex + 1) {
         led->flashOk();
      } else if (tkArgs.indexOf("error") == nCmdIndex + 1) {
         led->flashError();
      } else if (tkArgs.indexOf("busy") == nCmdIndex + 1) {
         led->flashBusy();
      } else if (tkArgs.indexOf("flash") == nCmdIndex + 1) {
         led->flashFlash();
      } else if (tkArgs.indexOf("data") == nCmdIndex + 1) {
         led->flashData();
      } else if (tkArgs.indexOf("wait") == nCmdIndex + 1) {
         led->flashWait();
      } else if (tkArgs.indexOf("connect") == nCmdIndex + 1) {
         led->flashConnect();
      } else {
         led->setFlash(TKTOINT(tkArgs, nCmdIndex + 1, 250), TKTOINT(tkArgs, nCmdIndex + 2, 128), TKTOINT(tkArgs, nCmdIndex + 3, 1));
      }
   } else if (tkArgs.indexOf("invert") == nCmdIndex) {
      if (tkArgs.count() > nCmdIndex + 1) {
         led->setInverted(TKTOINT(tkArgs, nCmdIndex + 1, false));
      } else {
         led->setInverted(!led->isInverted());
         led->toggle();
      }
   } else if (tkArgs.indexOf("toggle") == nCmdIndex) {
      led->toggle();
   } 
}
void help_led() {
   __console.println(F("led <subcmd> [args]"));
   __console.println(F("led <name <subcmd> [args]"));
   __console.println(F("  subcmd: on, off, blink, flash, invert, toggle"));
   __console.println(F("  on - turn on the LED"));
   __console.println(F("  off - turn off the LED"));
   __console.println(F("  blink [pattern] - blink the LED with a pattern (ok, error, busy, flash, data, wait, connect) or custom blink rate in ms and brightness (default is 1000ms and 128)"));
   __console.println(F("  flash [pattern] - flash the LED with a pattern (ok, error, busy, flash, data, wait, connect) or custom flash rate in ms and brightness (default is 250ms and 128)"));
   __console.println(F("  invert [true|false] - invert the LED logic"));
   __console.println(F("  toggle - toggle the LED state"));
}  

// Command sensor
void cmd_sensor(CxStrToken& tkArgs) {
   if (tkArgs.indexOf("list") == 1) {
      __sensorManager.printList();
   } else if (tkArgs.indexOf("name") == 1) {
      uint8_t nId = TKTOINT(tkArgs, 2, INVALID_UINT8);
      if (nId != INVALID_UINT8) {
         __sensorManager.setSensorName(nId, TKTOCHAR(tkArgs, 3));
      } else {
         __console.println(F("usage: sensor name <id> <name>"));
         __console.setExitValue(EXIT_FAILURE);
      }
   } else if (tkArgs.indexOf("get") == 1) {
      float f = __sensorManager.getSensorValueFloat(TKTOFLOAT(tkArgs, 2, INVALID_FLOAT));
      if (!std::isnan(f)) {
         __console.println(f);
         __console.setOutputVariable(f);
      } else {
         __console.println(F("invalid sensor id or value!"));
      }
   } else if (tkArgs.indexOf("add") == 1 && tkArgs.count() > 5) {
      // sensor add <name> <type> <unit> <variable> [friendly nama]
      CxSensor* pSensor = __sensorManager.getSensor(TKTOCHAR(tkArgs, 2));

      if (pSensor) {
      } else {
         pSensor = new CxSensorGeneric(TKTOCHAR(tkArgs, 2), ECSensorType::other, TKTOCHAR(tkArgs, 4), [](String& strParam) -> float {
            // Generic sensor callback. Value is read from a variable.
            if (strParam.length() > 0) {
               float fValue = 0.0F;
               char* end = nullptr;
               const char* szValue = __console.getVariable(strParam.c_str());
               if (szValue) {
                  fValue = std::strtod(szValue, &end);  // return as uint32_t with auto base
               }

               // Check if the conversion failed (no characters processed or out of range), then concat two strings
               if (end && end != szValue && *end == '\0') {
                  return fValue;
               }
            }
            return INVALID_FLOAT; }, TKTOCHAR(tkArgs, 5));
         if (pSensor) {
            pSensor->setTypeSz(TKTOCHAR(tkArgs, 3));
            pSensor->setFriendlyName(TKTOCHAR(tkArgs, 6));
         }
      }
   } else if (tkArgs.indexOf("del") == 1) {
      __sensorManager.removeSensor(TKTOCHAR(tkArgs, 2));
   }
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
void cmd_relay(CxStrToken& tkArgs) {

   CxGPIODevice* pDev = __gpioDeviceManager.getDevice(tkArgs.count() > 1 ? TKTOCHAR(tkArgs, 1) : "");

   if (tkArgs.indexOf("list") == 1) {
      __gpioDeviceManager.printList("relay");
   } else if (pDev) {
      if (pDev->getTypeSz() && strcmp(pDev->getTypeSz(), "relay") != 0) {
         __console.println(F("device is not a relay!"));
      } else {
         CxRelay* p = static_cast<CxRelay*>(pDev);

         if (tkArgs.indexOf("on") == 2) {
            p->on();
         } else if (tkArgs.indexOf("off") == 2) {
            p->off();
         } else if (tkArgs.indexOf("toggle") == 2) {
            p->toggle();
         } else if (tkArgs.indexOf("offtimer") == 2) {
            p->setOffTimer(TKTOINT(tkArgs, 3, 0));
         } else if (tkArgs.indexOf("default") == 2) {
            p->setDefaultOn(TKTOINT(tkArgs, 3, 0));
         } else {
            __console.println(F("invalid relay command"));
            __console.setExitValue(EXIT_FAILURE);
         }
      }
   }
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
void cmd_smooth(CxStrToken& tkArgs) {
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
      __console.setExitValue(EXIT_FAILURE);
   } else {
      float fValue = smoothRobust(reference, value, maxDiff, threshold, minAlpha, maxAlpha);
      __console.setOutputVariable(fValue);
      if (std::isnan(fValue)) __console.setExitValue(EXIT_FAILURE); // consider as non success, if nan was returned.
   }
}

// Command max
void cmd_max(CxStrToken& tkArgs) {
   // max <value1> <value2> [<value3> ...]
   // returns the maximum value of the given values
   // sets the output variable $> to the maximum value
   // sets the exit value to 0, if valid

   float fMax = TKTOFLOAT(tkArgs, 1, INVALID_FLOAT);
   if (std::isnan(fMax)) {
      __console.setExitValue(EXIT_FAILURE);
   }

   for (size_t i = 2; i < tkArgs.count(); i++) {
      float fValue = TKTOFLOAT(tkArgs, i, INVALID_FLOAT);
      if (!std::isnan(fValue)) {
         fMax = std::max(fMax, fValue);
      }
   }

   __console.setOutputVariable(fMax);
}

// Command min
void cmd_min(CxStrToken& tkArgs) {
   // min <value1> <value2> [<value3> ...]
   // returns the minimum value of the given values
   // sets the output variable $> to the minimum value
   // sets the exit value to 0, if valid

   float fMin = TKTOFLOAT(tkArgs, 1, INVALID_FLOAT);
   if (std::isnan(fMin)) {
      __console.setExitValue(EXIT_FAILURE);
   }
   for (size_t i = 2; i < tkArgs.count(); i++) {
      float fValue = TKTOFLOAT(tkArgs, i, INVALID_FLOAT);
      if (!std::isnan(fValue)) {
         fMin = std::min(fMin, fValue);
      }
   }
   __console.setOutputVariable(fMin);
}

// Command processdata
void cmd_processdata(CxStrToken& tkArgs) {
   if (tkArgs.indexOf("json") == 1 && tkArgs.count() > 3) {
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
   } else if (tkArgs.indexOf("list") == 1) {
      CxTablePrinter table(getIoStream());
      table.printHeader({F("Json Path"), F("Command")}, {20, 40});
      for (const auto& pair : __mapProcessJsonDataItems) {
         table.printRow({pair.first, pair.second.c_str()});
      }
   }
}

// Command table in PROGMEM
const CommandEntry commandsExt[] PROGMEM = {
    {"gpio", cmd_gpio, HELP_OR_NULLPTR(help_gpio)},
    {"led", cmd_led, HELP_OR_NULLPTR(help_led)},
    {"sensor", cmd_sensor, HELP_OR_NULLPTR(help_sensor)},
    {"relay", cmd_relay, HELP_OR_NULLPTR(help_relay)},
    {"smooth", cmd_smooth, nullptr},
    {"max", cmd_max, nullptr},
    {"min", cmd_min, nullptr},
    {"processdata", cmd_processdata, nullptr},
    // Add more extended commands here
};

const size_t NUM_COMMANDS_EXT = sizeof(commandsExt) / sizeof(commandsExt[0]);

#endif