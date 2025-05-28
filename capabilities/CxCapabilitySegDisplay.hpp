/**
 * @file CxCapabilitySegDisplay.hpp
 * @brief Defines classes for managing segment displays and their capabilities in an embedded system.
 *
 * This file contains the following classes:
 * - CxSegScreen: Represents a generic segment screen with properties and methods for display management.
 * - CxCapabilitySegDisplay: Manages segment display capabilities, including initialization, screen updates, and command execution.
 * - CxSegScreenOneValue: Represents a screen displaying a single value.
 * - CxSegScreenTime: Represents a screen displaying the current time.
 * - CxSegScreenStatic: Represents a static screen, optionally with blinking.
 * - CxSegScreenOneSensor: Represents a screen displaying a sensor value.
 *
 * The code includes conditional compilation for Arduino-specific libraries and functions.
 *
 * @date Created by ocfu on 09.03.25.
 * @copyright © 2025 ocfu. All rights reserved.
 *
 * @note ./Dev/Arduino/libraries/TM1637TinyDisplay/TM1637TinyDisplay.cpp modified:
 *      const uint8_t asciiToSegment[] PROGMEM = {
 *      ...
 *      0b00100100, // 037 %  // modified och
 *      ...
 *      0b01100011, // 126 ~  // modified och, used for deg sign
 *
 * https://github.com/jasonacox/TM1637TinyDisplay
 */

#ifndef CxCapabilitySegDisplay_hpp
#define CxCapabilitySegDisplay_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityFS.hpp"

#include "../tools/CxGpioTracker.hpp"
#include "../tools/CxTimer.hpp"

#ifdef ARDUINO
#include "TM1637TinyDisplay.h"
#undef isHigh
#undef isLow
#else
#define MAXDIGITS 4
#define SEG_A   0b00000001
#define SEG_B   0b00000010
#define SEG_C   0b00000100
#define SEG_D   0b00001000
#define SEG_E   0b00010000
#define SEG_F   0b00100000
#define SEG_G   0b01000000
#define SEG_DP  0b10000000
#endif

#define TM_DOTS 0b01000000

#include <map>
#include <cmath>

#ifndef ARDUINO
class TM1637TinyDisplay {};
#endif

class CxCapabilitySegDisplay;///< Forward declaration to break circular dependency

/**
 * @class CxSegScreen
 * @brief Represents a generic segment screen with properties and methods for display management.
 */
class CxSegScreen {
   friend CxCapabilitySegDisplay;
   
   /**
    * @var _pDisplay
    * @brief A pointer to the display object.
    */
   CxCapabilitySegDisplay* _pDisplay = nullptr;
   
   uint8_t _nId = 0; ///< The screen ID

   String _strName; ///< The screen name
   
   /**
    * @var _strParam
    * @brief The screen parameter
    * @details The screen parameter is a string that can be used to pass additional information to the screen.
    * For example, the parameter can be used to specify the sensor ID for a screen that displays sensor data.
    * The parameter is set using the setParam() method and retrieved using the getParam() method.
    */
   String _strParam;
   
protected:
   /**
    * @var __nOptionSeg
    * @brief The option segment indicator
    * @details The option segment indicator is used to specify a segment that should be displayed differently to indicate an option.
    * For example, the option segment can be used to display a dot or other symbol to indicate a setting or mode.
    */
   uint8_t __nOptionSeg = 0;
   void setId(uint8_t set) {_nId = set;}
   
public:
   
   virtual ~CxSegScreen() = default; ///< Virtual destructor
   
   /**
    * @brief Displays the screen.
    * @details This method is called to display the screen content.
    * The implementation of this method should update the display with the screen content.
    */
   virtual void show() {}
   
   /**
    * @brief Checks if the screen is empty.
    * @return True if the screen is empty, false otherwise.
    * @details This method is called to check if the screen is empty.
    * The implementation of this method should return true if the screen has no content to display.
    *
    */
   virtual bool isEmpty() = 0;
   
   /**
    * @brief Sets the screen parameter.
    * @param set The parameter to set.
    * @details This method is called to set the screen parameter.
    * The parameter is a string that can be used to pass additional information to the screen.
    * For example, the parameter can be used to specify the sensor ID for a screen that displays sensor data.
    */
   void setParam(const char* set) {if (set) _strParam = set;}
   const char* getParam() {return _strParam.c_str();}
   
   /**
    * @brief Sets the screen name.
    * @param set The name to set.
    * @details This method is called to set the screen name.
    * The name is a string that can be used to identify the screen.
    */
   void setName(const char* set) {_strName = set;}
   const char* getName() {return _strName.c_str();}

   /**
    * @brief Gets the screen type.
    * @return The screen type as a string.
    * @details This method is called to get the screen type.
    * The implementation of this method should return a string representing the screen type.
    * For example, the screen type can be "time" for a screen that displays the current time.
   */
   virtual const char* getType() = 0;
   
   uint8_t getId() {return _nId;}
   
   void setDisplay(CxCapabilitySegDisplay* set) {_pDisplay = set;}
   CxCapabilitySegDisplay* getDisplay() {return _pDisplay;}
   bool hasDisplay() {return _pDisplay != NULL;}
   
   /**
    * @brief Sets the option segment indicator.
    * @param opt The option segment indicator to set.
    * @details This method is called to set the option segment indicator.
    * The option segment indicator is used to specify a segment that should be displayed differently to indicate an option.
    * For example, the option segment can be used to display a dot or other symbol to indicate a setting or mode.
    */
   void setOption(uint8_t opt) {
      switch (opt) {
            // limit the options for the option indicator
         case SEG_F:
         case SEG_E:
         case SEG_A:
         case SEG_D:
            __nOptionSeg = opt;
            break;
      }
   };
};


/**
 * @class CxSegScreenOneValue
 * @brief Represents a screen displaying a single value.
 * @details This class extends CxSegScreen and represents a screen that displays a single value.
 * The value can be an integer, float, or string.
 * The screen content is updated by calling the show() method.
 * The screen can be configured to display the value with a specified number of digits and a decimal point.
 */
class CxCapabilitySegDisplay : public CxCapability {
   
   /**
    * @var sensors
    * @brief The sensor manager object.
    * @details The sensor manager object is used to access sensor data.
    * The sensor manager object is initialized with the sensor manager singleton instance.
    * The sensor manager is used to get sensor data for display on the segment display.
    */
   CxSensorManager& _sensors = CxSensorManager::getInstance();

protected:
   /**
    * @var __console
    * @brief The console object.
    * @details The console object is used to access the console for logging and output.
    * The console object is initialized with the console singleton instance.
    */
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();

public:
   /**
    * @enum ESegDisplayType
    * @brief Enumeration of segment display types.
    */
   enum class ESegDisplayType {
      none = 0,
      TM1637
   };

   /**
    * @enum ESegDisplayMode
    * @brief Enumeration of segment display modes.
    */
   enum class ESegDisplayMode {
      none = 0,
      time,
      data
   };
   
private:
   bool _bEnabled = false;
   bool _bDisableUpdate = false;
   
   /**
    * @var _buffer
    * @brief The display buffer.
    * @details The display buffer is an array of characters used to store the display content.
    * The buffer is updated with the screen content and then displayed on the segment display.
    */
   char _buffer[255];
   
   ESegDisplayType _eType;
   ESegDisplayMode _eMode = ESegDisplayMode::none;
   
   /**
    * @var _ptm1637
    * @brief The TM1637 display object.
    * @details The TM1637 display object is used to control the TM1637 segment display.
    */
   TM1637TinyDisplay* _ptm1637 = NULL;
   
   /**
    * @var _gpioClk
    * @brief The clock GPIO object.
    * @details The clock GPIO object is used to control the clock signal for the segment display.
    */
   CxGPIO _gpioClk;
   CxGPIO _gpioData;
   int _nBrigthness = 10;
   int _nBrightnessDefault = 10;
   
   CxTimer _timerSlideShow;
   CxTimer _timerMsg;
   CxTimer1s _timerUpdate;
   
   /**
    * @var _mapScreens
    * @var _mapScreensƒ
    * @brief A map of segment screens.
    * @details The map of segment screens is used to store the screens that are registered with the segment display capability.
    */
   std::map<String, std::unique_ptr<CxSegScreen>> _mapScreens;
   std::vector<uint8_t> _vScreenSlideShow;
   
   int _nActiveScreenIndex = 0;
   bool _bSlideShowOn = false;
   int _nStartScreen = -1;
   
   unsigned _nBlinkCnt = 0;
   int _nBrigthnessPrev;


public:
   /**
    * @brief Default constructor.
    * @details Initializes the segment display capability with the "seg" command.
    */
   explicit CxCapabilitySegDisplay()
   : CxCapability("seg", getCmds()) {}
   /**
    * @brief Gets the name of the segment display capability.
    * @return The name of the segment display capability.
    */
   static constexpr const char* getName() { return "seg"; }
   /**
    * @brief Gets the commands supported by the segment display capability.
    * @return A vector of commands supported by the segment display capability.
    */
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "seg" };
      return commands;
   }
   /**
    * @brief Constructs a segment display capability with the given parameters.
    * @param param The parameters for the segment display capability.
    * @return A unique pointer to the constructed segment display capability.
    * @details This method constructs a segment display capability with the given parameters
    */
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilitySegDisplay>();
   }

   /**
    * @brief Destructor.
    * @details Stops the segment display capability and clears the screen buffer.
    *
    */
   ~CxCapabilitySegDisplay() {
      end();
      _bEnabled = false;
      _mapScreens.clear();
   }
   
   /**
    * @brief Initializes the segment display capability.
    * @details Sets up the segment display capability and initializes the display object.
    * The method also loads the segment display screens and sets the brightness level.
    */
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*__console.getStream());
      __bLocked = false;
      
      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());
          
      __console.executeBatch("init", getName());

   }
   
   static CxCapabilitySegDisplay* getInstance() {
      return static_cast<CxCapabilitySegDisplay*>(ESPConsole.getCapInstance("seg"));
   }

   /**
    * @brief Loops the segment display capability.
    * @details Updates the segment display content and handles display updates.
    * The method also handles screen transitions and slide show functionality.
    */
   void loop() override {
      if (_bEnabled) {
         // update timer for display
         if (_timerUpdate.isDue()) {
            // get active screen
            CxSegScreen *pScreen = findScreen(getActiveScreenIndex());
            
            // show screen
            if (pScreen) {
               if (!_bDisableUpdate) pScreen->show();
            } else {
               // clears the screen, if the index has no screen registered.
               if (!_bDisableUpdate) clear();
            }
            
            int nBr = getBrightness();
            
            // blink
            if (_nBlinkCnt > 0) {
               if (_nBlinkCnt % 2 == 0) {
                  _nBrigthnessPrev = getBrightness();
                  if (_nBrigthnessPrev > 50) {
                     nBr = 20;
                  } else {
                     nBr = 100;
                  }
               } else {
                  nBr = _nBrigthnessPrev;
               }
               
               if (_nBlinkCnt < 0xfff0) {
                  _nBlinkCnt--;
               } else {
                  _nBlinkCnt = (_nBlinkCnt == 0xfff2) ? 0xfff1 : 0xfff2; // endless blinking
               }
            }
            
            if (nBr != getBrightness()) {
               setBrightness(nBr);
            }
         }
         
         // message timer
         if (_timerMsg.isDue()) {
            _timerMsg.stop();
            _bDisableUpdate = false;
         }
         
         // slide show
         if (_timerSlideShow.isDue()) {
            if (isSlideShowEnabled() && _vScreenSlideShow.size() > 0) {
               static uint8_t nIndex = 0;
               
               // get screen
               CxSegScreen *pScreen = findScreen(_vScreenSlideShow[nIndex]);
               if (pScreen != nullptr && !pScreen->isEmpty()) {
                  setActiveScreenIndex(_vScreenSlideShow[nIndex]);
               }
               
               // next screen
               nIndex = (++nIndex >= _vScreenSlideShow.size() ? 0 : nIndex);
            }
         }
      }
   }

   /**
    * @brief Initializes the segment display capability.
    * @details Sets up the segment display capability and initializes the display object.
    * The method also loads the segment display screens and sets the brightness level.
    * @return True if the initialization is successful, otherwise false.
    *
    */
   bool execute(const char *szCmd, uint8_t nClient) override {
      // validate the call
      if (!szCmd) return false;
      
      // get the command and arguments into the token buffer
      CxStrToken tkCmd(szCmd, " ");
      
      // validate again
      if (!tkCmd.count()) return false;
      
      // we have a command, find the action to take
      String strCmd = TKTOCHAR(tkCmd, 0);
      
      // removes heading and trailing white spaces
      strCmd.trim();
            
      if ((strCmd == "?")) {
         printCommands();
      } else if (strCmd == "seg") {
         String strSubCmd = TKTOCHAR(tkCmd, 1);
         strSubCmd.toLowerCase();
         String strEnv = ".seg";
         if (strSubCmd == "enable") {
            _bEnabled = (bool)TKTOINT(tkCmd, 2, 0);
            if (_bEnabled) init();
         } else if (strSubCmd == "list") {
            printScreens();
         } else if (strSubCmd == "test") {
            test();
         } else if (strSubCmd == "br") {
            setBrightness(TKTOINT(tkCmd, 2, _nBrigthness));
         } else if (strSubCmd == "print") {
            segprint(TKTOCHAR(tkCmd, 2));
         } else if (strSubCmd == "clear") {
            clear();
         } else if (strSubCmd == "on") {
            on();
         } else if (strSubCmd == "off") {
            off();
         } else if (strSubCmd == "blink") {
            blink(TKTOINT(tkCmd, 2, 0));
         } else if (strSubCmd == "msg") {
            showMsg(TKTOCHAR(tkCmd, 2), TKTOINT(tkCmd, 3, 5000));
         } else if (strSubCmd == "opt") {
            showOption(TKTOINT(tkCmd, 2, 0));
         } else if (strSubCmd == "level") {
            showLevel(TKTOINT(tkCmd, 2, 0), TKTOINT(tkCmd, 3, true));
         } else if (strSubCmd == "setpins" && (tkCmd.count() >= 3)) {
            setPins(TKTOINT(tkCmd, 2, -1), TKTOINT(tkCmd, 3, -1));
         } else if (strSubCmd == "screen") {
            String strFunc = TKTOCHAR(tkCmd, 2);
            if (strFunc == "add" && tkCmd.count() > 4) {
               String strName = TKTOCHAR(tkCmd, 3);
               if (strName == "sensors") {
                  uint8_t n = _sensors.getSensorCount();
                  for (uint8_t i = 0; i < n; i++) {
                     CxSensor* pSensor = _sensors.getSensor(i);
                     if (pSensor) {
                        addScreen(pSensor->getTypeSz(), "sensor", pSensor->getName());
                     }
                  }
               } else {
                  addScreen(TKTOCHAR(tkCmd, 3), TKTOCHAR(tkCmd, 4), TKTOCHAR(tkCmd, 5));
               }
            } else if (strFunc == "del" && tkCmd.count() > 3) {
               delScreen(TKTOCHAR(tkCmd, 3));
            } else {
               __console.println(F("seg screen commands:"));
               __console.println(F("  add <name> <type> [<id>]"));
               __console.println(F("  del <name>"));
               __console.println(F("  add sensors"));
            }
         } else if (strSubCmd == "show") {
            setActiveScreenIndex(TKTOINT(tkCmd, 2, INVALID_UINT8));
         } else if (strSubCmd == "slideshow") {
            String strFunc = TKTOCHAR(tkCmd, 2);
            strFunc.toLowerCase();
            if (strFunc == "add") {
               uint8_t n = TKTOINT(tkCmd, 3, INVALID_UINT8);
               if (n != INVALID_UINT8) {
                  _vScreenSlideShow.push_back(n);
               }
            } else if (strFunc == "del") {
               uint8_t n = TKTOINT(tkCmd, 3, INVALID_UINT8);
               if (n != INVALID_UINT8) {
                  for (auto it = _vScreenSlideShow.begin(); it != _vScreenSlideShow.end(); ++it) {
                     if (*it == n) {
                        _vScreenSlideShow.erase(it);
                        break;
                     }
                  }
               }
            } else if (strFunc == "list") {
               __console.println(F("Slide show:"));
               for (uint8_t n : _vScreenSlideShow) {
                  CxSegScreen* pScreen = findScreen(n);
                  if (pScreen != nullptr) {
                     __console.printf(F("  %02d %s %s\n"), n, pScreen->getName(), pScreen->getType());
                  }
               }
            } else if (strFunc == "on") {
               _bSlideShowOn = true;
            } else if (strFunc == "off") {
               _bSlideShowOn = false;
            } else {
               __console.man(strSubCmd.c_str());
            }
         } else if (strSubCmd == "init") {
            init();
         } else {
            printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), _bEnabled);
            printf(F(ESC_ATTR_BOLD " Brightness:   " ESC_ATTR_RESET "%d\n"), _nBrigthness);
            printf(F(ESC_ATTR_BOLD " Slide show:   " ESC_ATTR_RESET "%s\n"), (_bSlideShowOn ? "on" : "off"));
            printf(F(ESC_ATTR_BOLD " Screens:      " ESC_ATTR_RESET "%d\n"), _mapScreens.size());
            __console.man(getName());
         }
      } else {
         // command not handled here
         return false;
      }
      g_Stack.update(); ///< Update the stack status after executing the command
      return true;
   }
   
   /**
    * @brief Initializes the segment display capability.
    * @details Sets up the segment display capability and initializes the display object.
    * The method also loads the segment display screens and sets the default brightness level.
    * @return True if the initialization is successful, otherwise false.
    *
    */
   bool init() {
      if (_bEnabled) {
         end();
         _CONSOLE_INFO(F("7SEG: start segment display..."));
         if (Led1.getPin() == _gpioClk.getPin() || Led1.getPin() == _gpioData.getPin()) {
            _CONSOLE_INFO(F("7SEG: disable Led1, use of same gpio %d."), Led1.getPin());
            Led1.setPin(-1);
         }
#ifdef ARDUINO
         _ptm1637 = new TM1637TinyDisplay(_gpioClk.getPin(), _gpioData.getPin());
#else
         _ptm1637 = new TM1637TinyDisplay;
#endif
         if (_ptm1637 != NULL) {
            // All segments on and off
            clear();
            setBrightness(100);
#ifdef ARDUINO
            _ptm1637->showNumber(8888);
#endif
            delay(500);
            setBrightness(10);
#ifdef ARDUINO
            _ptm1637->showNumber(8888);
            _ptm1637->setScrolldelay(200);
#endif
            delay(500);
            setBrightness(getBrightnessDefault());
            clear();
            
            if (getStartScreen() >= 0) {
               setActiveScreenIndex(getStartScreen());
            }
            
            _timerSlideShow.start(5000, false);
            _timerMsg.start(5000, false);

            _CONSOLE_INFO(F("7SEG: ready"));
            return true;
         } else {
            __console.error(F("7SEG: ### start failed!"));
            return false;
         }
      }
      return false;
   }

   bool init(int nClkPin, int nIOPin) {
      setPins(nClkPin, nIOPin);
      return init();
   };

   void end() {
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         _ptm1637->clear();
         delete _ptm1637;
#endif
      }
   }

   // Function to list all screens in the map
   void printScreens() {
      if (_mapScreens.empty()) {
         println(F("No screens registered."));
         return;
      }
      
      println(F(ESC_ATTR_BOLD "Seg screens: " ESC_ATTR_RESET));
      for (const auto& [name, screen] : _mapScreens) {
         printf(ESC_TEXT_WHITE " %02d %s %s\n" ESC_ATTR_RESET, screen->getId(), name.c_str(), screen->getType());
      }
   }
   
   bool hasValidPins() {return (_gpioClk.isValid() && _gpioData.isValid() && _gpioClk.getPin() != _gpioData.getPin());}
   
   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   void setPins(int nClkPin, int nDataPin) { ///< Set the clock and data pins for the segment display
      _gpioClk.setPin(nClkPin);
      _gpioClk.setPinMode(OUTPUT);
      _gpioClk.setGpioName("clk");
      _gpioData.setPin(nDataPin);
      _gpioData.setPinMode(OUTPUT);
      _gpioData.setGpioName("data");
   }
   
   CxGPIO& getGPIOData() {return _gpioData;}
   CxGPIO& getGPIOClk() {return _gpioClk;}

   void setBrightness(int nBr) { ///< Set the brightness level for the segment display (0-100) and update the display
      if (nBr > 100) nBr = 100;
      if (nBr < 0) nBr = 0;
      _nBrigthness = nBr;
      
      if (_nBrigthness == 0) {
         off();
      } else {
         on();
      }
   }
   
   int getBrightness() {return _nBrigthness;}
   
   void setBrightnessDefault(int set = 30) {_nBrightnessDefault = set;_nBrigthnessPrev = set; setBrightness(set);}
   int getBrightnessDefault() {return _nBrightnessDefault;}
   
   void clear() {
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         _ptm1637->clear();
#else
         std::cout << "7SEG: (clear)" << "\n";
#endif
      }
   }
   
   void on() {
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         _ptm1637->setBrightness(7 * _nBrigthness/100, true);
#else
         std::cout << "7SEG: on" << "\n";
#endif
      }
   }
   
   void off() {
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         _ptm1637->setBrightness(7 * _nBrigthness/100, false);
#else
         std::cout << "7SEG: brightness=" << (7*_nBrigthness/100) << "\n";
#endif
      }
   }
   
   void segprint(int16_t n) {
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         _ptm1637->showNumber(n);
#else
         std::cout << "7SEG: '" << n << "'\n";
#endif
      }
   }
   
   void segprint(const char *sz) {
      if (_ptm1637 != NULL) {
         
#ifdef ARDUINO
         _ptm1637->showString(::remove8BitChars(sz));
#else
         std::cout << "7SEG: '" << ::remove8BitChars(sz) << "'\n";
#endif
      }
   }
   
   void segprint(const FLASHSTRINGHELPER* sz) {
#ifdef ARDUINO
      strncpy_P(_buffer, (PGM_P)sz, sizeof(_buffer));
#else
      strncpy(_buffer, (PGM_P)sz, sizeof(_buffer));
#endif
      segprint(_buffer);
   }
   
   void segprintf(const char *fmt...) {
      if (_ptm1637 != NULL) {
         va_list args;
         va_start(args, fmt);
         vsnprintf (_buffer, sizeof(_buffer), fmt, args);
         segprint(_buffer);
         va_end(args);
      }
   }
   
   void segprintf(const FLASHSTRINGHELPER *fmt...) {
      if (_ptm1637 != NULL) {
         va_list args;
         va_start(args, fmt);
#ifdef ARDUINO
         vsnprintf (_buffer, sizeof(_buffer), (PGM_P)fmt, args);
#else
         vsnprintf (_buffer, sizeof(_buffer), (PGM_P)fmt, args);
#endif
         segprint(_buffer);
         va_end(args);
      }
   }

   void showNumber(int16_t number, bool zeroPadding = false, bool rollOver = false, bool alignLeft = false) {
      const int16_t maxNumber = 9999;
      const int16_t minNumber = -999;
      bool positive = true;
      int sign = 0;
      
      // get and store sign
      if (number < 0) {
         positive = false;
         sign = 1;
      };
      
      // roll over if rollOver is set to true
      if (rollOver == true) {
         if (positive == true) {
            number = number % int16_t(10000);
         } else {
            number = -1 * ((-1 * number) % 1000);
         }
         // limit number by default
      } else {
         number = number > maxNumber?maxNumber:number;
         number = number < minNumber?minNumber:number;
      }
      
      // align right is the default behavior, just forward to print
      if (alignLeft == false || number < -999 || number > 999) {
#ifdef ARDUINO
         _ptm1637->showNumber(number, zeroPadding, 4, 0);
#else
         std::cout << "7SEG: '" << number << "'\n";
#endif
         return;
      }
      
      clear();
      
      if (number < -99 || number > 99) {
#ifdef ARDUINO
         _ptm1637->showNumber(number, false, 3+sign, 0);
#else
         std::cout << "7SEG: ' " << number << "'\n";
#endif
      } else if (number < -9 || number > 9) {
#ifdef ARDUINO
         _ptm1637->showNumber(number, false, 2+sign, 0);
#else
         std::cout << "7SEG: '  " << number << "'\n";
#endif
      } else {
#ifdef ARDUINO
         _ptm1637->showNumber(number, false, 1+sign, 0);
#else
         std::cout << "7SEG: '   " << number << "'\n";
#endif
      };
   }
   
   void showNumberCentred(int n) {
      if (_ptm1637 != NULL) {
         if ((n < -99) || n > 999) {
            showNumber(n);
         } else {
            //clear();
#ifdef ARDUINO
            _ptm1637->showNumber(n, false, 3, 0);
#else
            std::cout << "7SEG: '" << n << "'\n";
#endif
         }
      }
   }
   
   void showString(const char* sz, uint8_t length = MAXDIGITS, uint8_t pos = 0, uint8_t dots = 0) {
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         _ptm1637->showString(sz, length, pos, dots);
#else
         std::cout << "7SEG: '" << sz << "'\n";
#endif
      }
   }
   
   /**
    * @brief Shows the time on the segment display.
    * @details Displays the current time on the segment display.
    * The time is displayed in the format "HH:MM" with a colon between the hours and minutes.
    */
   void showTime() {
      if (_ptm1637 != NULL) {
         static bool bColon = true;
         if (__console.isValid()) {
            
#ifdef ARDUINO
            int dots = (bColon) ? TM_DOTS : 0x0;
            _ptm1637->showNumberDec(__console.getTimeHour(), dots, true, 2, 0);
            _ptm1637->showNumberDec(__console.getTimeMin(), dots, true, 2, 2);
#else
            std::cout << "7SEG: '" << "01:23" << "'\n";
#endif
         } else {
            clear();  // use the object's clear(), this will not reset the colon.
            if (__console.isAPMode()) {
               segprint(" AP ");
            } else {
#ifdef ARDUINO
               segprint("----");
#else
               segprint("00:00");
#endif
            }
         }
         bColon = !bColon;
      }
   }
   
   void showSave() {
      clear();
      segprint("Save");
   }
   
   void showError() {
      clear();
      segprint("Err");
   }
   
   void showOn() {
      clear();
      segprint("on");
   }
   
   void showOff() {
      clear();
      segprint("off");
   }
   

   /**
    * @brief Tests the segment display capability.
    * @details Tests the segment display capability by displaying various numbers, messages, and symbols on the segment display.
    * The method displays numbers from -110 to 100, the time, the save message, the error message, the on message, and the off message.
    */
   void test() {
      if (_ptm1637 != NULL) {
         showNumber(8888);
         delay(1000);
         
#ifdef ARDUINO
         for (int x = -110; x <= 100; x++) {
            showNumber(x);
            yield(); // Keep watchdog timer happy
         }
         delay(500);
         clear();
         for (int x = -110; x <= 100; x++) {
            showNumber(x, false, false, true);
            yield(); // Keep watchdog timer happy
         }
         delay(500);
         
         // Demo Brightness Levels
         char string[10];
         for (int x = 0; x <= 100; x+=10) {
            setBrightness(x);
            showNumber(x);
            delay(500);
         }
         
#endif
         delay(1000);
         showTime();
         delay(1000);
         showSave();
         delay(1000);
         showError();
         delay(1000);
         showOn();
         delay(1000);
         showOff();
         delay(1000);
         clear();
      }
   }
   
   void modeTime() {_eMode = ESegDisplayMode::time;}
   void modeData() {_eMode = ESegDisplayMode::data;}
   void modeNone() {_eMode = ESegDisplayMode::none;}

   /**
    * @brief Adds a screen to the segment display.
    * @param szName The name of the screen to add.
    * @param szType The type of the screen to add.
    * @param szParam The parameters for the screen to add.
    */
   void addScreen(const char* szName, const char* szType, const char* szParam = nullptr);
   void delScreen(const char* szName) {
      _CONSOLE_DEBUG(F("7SEG: delete screen '%s'"), szName);
      _mapScreens.erase(szName);
   };

   void addScreen(const char* szName, std::unique_ptr<CxSegScreen> pScreen, const char* szParam = nullptr) {
      if (pScreen != nullptr) {
         _CONSOLE_DEBUG(F("7SEG: add screen '%s' with screen id %d."), szName, _mapScreens.size());
         pScreen->setDisplay(this);
         pScreen->setId(_mapScreens.size());
         pScreen->setParam(szParam);
         pScreen->setName(szName);
         _mapScreens[szName] = std::move(pScreen);
      }
   }

   /**
    * @brief Finds a screen by name.
    * @param szName The name of the screen to find.
    * @return A pointer to the screen with the given name, or nullptr if the screen is not found.
    */
   CxSegScreen* findScreen(const char* szName) {
      auto itScreen = _mapScreens.find(szName);
      if (itScreen != _mapScreens.end()) {
         return itScreen->second.get();
      }
      return nullptr;
   }

   /**
    * @brief Finds a screen by ID.
    * @param nId The ID of the screen to find.
    * @return A pointer to the screen with the given ID, or nullptr if the screen is not found.
    */
   CxSegScreen* findScreen(uint8_t nId) {
      for (const auto& [name, screen] : _mapScreens) {
         if (screen->getId() == nId) return _mapScreens[name].get();
      }
      return nullptr;
   }

   /**
      * @brief Shows a screen by ID.
      * @param id The ID of the screen to show.
    */
   void showScreen(uint8_t id) {
      CxSegScreen* pScreen = findScreen(id);
      if (pScreen) {
         clear();
         pScreen->show();
      }
   }
   
   /**
    * @brief Shows a screen by name.
    * @param szName The name of the screen to show.
    */
   void showScreen(const char* szName) {
      CxSegScreen* pScreen = findScreen(szName);
      if (pScreen != NULL) {
         clear();
         pScreen->show();
      }
   }
   
   void deleteAllScreens() {
      _mapScreens.clear();
   }
   
   void setStartScreen(int set) {_nStartScreen = set;}
   int getStartScreen() {return _nStartScreen;}
      
   void setActiveScreenIndex(int set) {_nActiveScreenIndex = set;}
   int getActiveScreenIndex() {return _nActiveScreenIndex;}
   void enableSlideShow(bool set) {_bSlideShowOn = set;}
   bool isSlideShowEnabled() {return _bSlideShowOn;}
   
   size_t getScreenCount() {return _mapScreens.size();}
   
   void blink(unsigned n) {_nBlinkCnt = 2*n; if(n==0)setBrightness(getBrightnessDefault());}
   void blink1() {blink(1);}
   void blink() {_nBlinkCnt = 0xfff2;} // endless blinking
   
   void showMsg(const char *szMsg, unsigned nRemain = 5000) {
      if (szMsg) {
         _timerMsg.start(nRemain, false);
         _bDisableUpdate = true;
         clear();
         segprint(szMsg);
      }
   }
   
   void showOption(uint8_t nOptSeg) {
      // use the segments e and f of the first digit to indicate two options (indicating the data shown in following digits)
      if (_ptm1637 != NULL) {
#ifdef ARDUINO
         if (nOptSeg) { // avoid flicker of the segment
            _ptm1637->setSegments(nOptSeg, 0);
         }
#else
         std::cout << "7SEG: option seg '" << nOptSeg << "'\n";
#endif
      }
   }
   
   /**
    * @brief Shows a level on the segment display.
    * @param level The level to show on the segment display.
    * @param horizontal True to show the level horizontally, false to show the level vertically.
    */
   void showLevel(unsigned int level = 100, bool horizontal = true) {
#ifdef ARDUINO
      _ptm1637->showLevel(level, horizontal);
#else
      std::cout << "7SEG: show level " << level << "\n";
#endif
   }
   
   static void loadCap() {
      CAPREG(CxCapabilitySegDisplay);
      CAPLOAD(CxCapabilitySegDisplay);
   };

};


/**
 * @brief The CxSegScreenOneValue class represents a screen on the segment display that displays a single value.
 * @details The CxSegScreenOneValue class represents a screen on the segment display that displays a single value.
 * The screen can display a string, a float value, or an integer value. The screen can also display a unit with the value.
 */
class CxSegScreenOneValue : public CxSegScreen {
protected:
   const char*  _pszUnit  = NULL;
   const char*  _pszValue = NULL;
   const float* _pfValue = NULL;
   const int*   _pnValue = NULL;
   
   float _fMinValue = 0.0;
   float _fMaxValue = 0.0;
   
   void setValuePtr(const char* set, const char* unit = NULL) {_pszValue = set; _pszUnit = unit;}
   void setValuePtr(const float* set, const char* unit = NULL) {_pfValue = set; _pszUnit = unit;}
   void setValuePtr(const int* set, const char* unit = NULL) {_pnValue = set; _pszUnit = unit;}
   
public:
   CxSegScreenOneValue() {}
   CxSegScreenOneValue(const char* set, const char* unit = NULL, unsigned opt = 0) {setValuePtr(set, unit);CxSegScreen::setOption(opt); }
   CxSegScreenOneValue(const float* set, const char* unit = NULL, unsigned opt = 0) {setValuePtr(set, unit);CxSegScreen::setOption(opt);}
   CxSegScreenOneValue(const int* set, const char* unit = NULL, unsigned opt = 0) {setValuePtr(set, unit);CxSegScreen::setOption(opt);}
   
   
   // screen is empty, if no value pointer has been set or if the string zero buffer is empty
   bool isEmpty() {
      if (_pszValue==NULL && _pfValue==NULL && _pnValue==NULL)
         return true;
      if (_pszValue != NULL) {
         if (_pszValue[0]=='\0')
            return true;
      }
      return false;
   }
   
   virtual const char* getType() { return "one";}

   bool hasMinMax() {return (_fMinValue != _fMaxValue);}

   /**
    * @brief Shows the screen on the segment display.
    * @details Shows the screen on the segment display by displaying the value of the screen.
    * The screen displays the value as a string, a float value, or an integer value.
    */
   void show() {
      CxSegScreen::show();
      if (hasDisplay()) {
         if (_pszValue != nullptr) {
            getDisplay()->printf("%s%s", _pszValue, (_pszUnit!=nullptr) ? _pszUnit : "");
         } else if (_pfValue != nullptr) {
            if (!hasMinMax() || (_fMinValue < *_pfValue && *_pfValue <= _fMaxValue)) {
               if (_pszUnit == nullptr)
                  getDisplay()->printf("%3.0f", *_pfValue);
               else
                  getDisplay()->printf("%3.0f%s", *_pfValue, _pszUnit);
            } else {
               getDisplay()->printf(" --%s", (_pszUnit!=nullptr) ? _pszUnit : "");
            }
         } else if (_pnValue != nullptr) {
            if (!hasMinMax() || ((int)_fMinValue < *_pnValue && *_pnValue <= (int)_fMaxValue)) {
               if (_pszUnit == nullptr)
                  getDisplay()->printf("%3d", *_pnValue);
               else
                  getDisplay()->printf("%3d%s", *_pnValue, _pszUnit);
            } else {
               getDisplay()->printf(" --%s", (_pszUnit!=nullptr) ? _pszUnit : "");
            }
         }
      }
   }
   
   void setMinValue(float set) {_fMinValue = set;}
   float getMinValue() {return _fMinValue;}
   void setMaxValue(float set) {_fMaxValue = set;}
   float getMaxValue() {return _fMaxValue;}
};

/**
 * @brief The CxSegScreenTime class represents a screen on the segment display that displays the time.
 * The screen displays the current time on the segment display in the format "HH:MM".
 */
class CxSegScreenTime : public CxSegScreen {
public:
   bool isEmpty() {return false;}  // this screen is never empty
   virtual const char* getType() { return "time";}

   void show() {
      CxSegScreen::show();
      if (hasDisplay()) {
         getDisplay()->showTime();
      }
   }
};

/**
 * @brief The CxSegScreenStatic class represents a screen on the segment display that displays a static value.
 * The screen displays a static value on the segment display.
 * The screen can also be set to blink.
 */
class CxSegScreenStatic : public CxSegScreen {
   bool _bBlinking = false;
   
public:
   CxSegScreenStatic() {};
   CxSegScreenStatic(bool bBlink) {setBlinking(bBlink);}
   
   bool isEmpty() {return true;}
   virtual const char* getType() { return "static";}

   void show() {
      if (hasDisplay() && isBlinking()) {
         getDisplay()->blink();
      }
   }
   
   void setBlinking(bool set) {_bBlinking = set;}
   bool isBlinking() {return _bBlinking;}
};

/**
 * @brief The CxSegScreenOneSensor class represents a screen on the segment display that displays a single sensor value.
 * The screen displays a single sensor value on the segment display.
 */
class CxSegScreenOneSensor : public CxSegScreenOneValue {
   CxSensor* _pSensor = NULL;
   
public:
   CxSegScreenOneSensor(CxSensor* p, unsigned opt = 0) {
      _pSensor = p;
      setOption(opt);
      
      if (hasSensor()) {
         setMinValue(getSensor()->getMinValue());
         setMaxValue(getSensor()->getMaxValue());
         _pszUnit = getSensor()->getUnit();
      }
   }
   
   bool isEmpty() {
      return !hasSensor();
   }
   virtual const char* getType() { return "sensor";}

   /**
    * @brief Shows the screen on the segment display.
    * @details Shows the screen on the segment display by displaying the value of the sensor.
    * The screen displays the sensor value as an integer value.
    */
   void show() {
      CxSegScreenOneValue::show();
      if (hasDisplay() && hasSensor()) {
         if (getSensor()->hasValidValue()) {
            const char* sz = getSensor()->getUnit();
            getDisplay()->showNumberCentred(getSensor()->getIntValue());
            // show sensor's unit
            if (getSensor()->getIntValue() > -99 && getSensor()->getIntValue() < 1000) {
               if (strncmp(sz, "°", 1) == 0) {
                  getDisplay()->showString("\xB0", 1, 3); // display degree character
               } else {
                  getDisplay()->showString(sz, 1, 3);
               }
               // some space left to show the option for the data
               getDisplay()->showOption(__nOptionSeg);
            }
         } else {
            getDisplay()->showString("--", 2, 1);
         }
      }
   };
   
   bool hasSensor() {
      return (_pSensor && _pSensor->isValid());
   }
   CxSensor* getSensor() {
      return _pSensor;
   }
   
};


/**
 * @brief Adds a screen to the segment display.
 * @param szName The name of the screen to add.
 * @param szType The type of the screen to add.
 * @param szParam The parameters for the screen to add.
 */
void CxCapabilitySegDisplay::addScreen(const char* szName, const char* szType, const char* szParam) {
   if (szName && szType) {
      String strType = szType;
      if (strType == "time") {
         addScreen(szName, std::make_unique<CxSegScreenTime>());
      } else if (strType == "static") {
         addScreen(szName, std::make_unique<CxSegScreenStatic>());
      } else if (strType == "one") {
         addScreen(szName, std::make_unique<CxSegScreenOneValue>());
      }  else if (strType == "sensor") {
         if (szParam) {
            CxSensor* pSensor = _sensors.getSensor(szParam); // get sensor by name
            if (pSensor == nullptr) {
               __console.error(F("7SEG: sensor '%s' was not found."), szParam);
            } else {
               _CONSOLE_INFO(F("7SEG: add sensor '%s' to screen '%s'"), szParam, szName);
               addScreen(szName, std::make_unique<CxSegScreenOneSensor>(pSensor), szParam);
            }
         } else {
            __console.error(F("7SEG: sensor screen needs a sensor name."));
         }
      }
   }
}


#endif /* CxCapabilitySegDisplay_hpp */

