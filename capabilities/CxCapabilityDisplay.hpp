/**
 * @file CxCapabilityDisplay.hpp
 * @brief Defines the Oled display capability for the ESP console.
 *
 * This capability adds an Oled display (currently oled128x32) to the project.
 * The display is connected to the I2C bus provided by the existing i2c
 * capability (CxCapabilityI2C) and controlled with Adafruit_GFX and
 * Adafruit_SSD1306.
 *
 * The main command is "disp" and is used to initialise and control the
 * display. For now "disp init" is the entry point which initialises the
 * display and shows an initialisation message.
 *
 * The display type can be extended later with further display types.
 *
 * @date Created by ocfu on 31.08.26.
 * @copyright © 2026 ocfu. All rights reserved.
 */

#ifndef CxCapabilityDisplay_hpp
#define CxCapabilityDisplay_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityFS.hpp"
#include "../capabilities/CxCapabilityI2C.hpp"

#include "../tools/CxTimer.hpp"

#ifdef ARDUINO
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#else
// stub for the native macOS build
class Adafruit_SSD1306 {
public:
   Adafruit_SSD1306(uint8_t w, uint8_t h, void*, int8_t = -1) {}
   bool begin(uint8_t = 0, uint8_t = 0, bool = true, bool = true) {return true;}
   void display() {}
   void clearDisplay() {}
   void invertDisplay(bool) {}
   void setTextSize(uint8_t) {}
   void setTextColor(uint16_t) {}
   void setCursor(int16_t, int16_t) {}
   void fillScreen(uint16_t) {}
   size_t print(const char* s) {std::cout << "OLED: '" << s << "'\n"; return 0;}
   size_t println(const char* s) {std::cout << "OLED: '" << s << "'\n"; return 0;}
};
#endif

// default I2C address of the Oled display if not found by the i2c scan
#ifndef OLED_DEFAULT_ADDR
#define OLED_DEFAULT_ADDR 0x3C
#endif

class CxCapabilityDisplay;

/**
 * @class CxDisplayInit
 * @brief Helper initialiser that initialises the display through the i2c
 * capability (CxInitializer interface).
 */
class CxDisplayInit : public CxInitializer {
   CxCapabilityDisplay* _pDisplay = nullptr;

public:
   CxDisplayInit(CxCapabilityDisplay* pDisplay) : _pDisplay(pDisplay) {}

   void init() override;
};

/**
 * @class CxCapabilityDisplay
 * @brief Manages the Oled display capability for the ESP console.
 */
class CxCapabilityDisplay : public CxCapability {

protected:
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();

public:
   /**
    * @enum EOLEDType
    * @brief Enumeration of supported Oled display types.
    */
   enum class EOLEDType {
      none = 0,
      oled128x32
   };

private:
   bool _bEnabled = false;
   bool _bInit = false;

   EOLEDType _eType = EOLEDType::none;

   uint8_t _nWidth = 128;
   uint8_t _nHeight = 32;
   uint8_t _nAddr = OLED_DEFAULT_ADDR;

   Adafruit_SSD1306* _pDisplay = nullptr;

   CxDisplayInit _init;

public:
   /**
    * @brief Default constructor.
    * @details Initialises the display capability with the "disp" command and
    * registers itself as an I2C initialiser, so it is initialised when the
    * i2c capability is initialised.
    */
   explicit CxCapabilityDisplay()
   : CxCapability("disp", getCmds()), _init(this) {
      _eType = EOLEDType::oled128x32;
      begin();
   }

   static constexpr const char* getName() { return "disp"; }

   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "disp" };
      return commands;
   }

   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityDisplay>();
   }

   /**
    * @brief Destructor.
    * @details Ends the display capability and deletes the display object.
    */
   ~CxCapabilityDisplay() {
      end();
      _bEnabled = false;
   }

   /**
    * @brief Initialises the display capability.
    * @details Sets up the display capability and routes to the user batch
    * file label "disp" for configuration.
    */
   void setup() override {
      CxCapability::setup();

      setIoStream(*__console.getStream());

      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());

      __console.executeBatch("init", getName());
   }

   static CxCapabilityDisplay* getInstance() {
      return static_cast<CxCapabilityDisplay*>(ESPConsole.getCapInstance("disp"));
   }

   /**
    * @brief Loops the display capability.
    */
   void loop() override {
   }

   /**
    * @brief Executes a command for the display capability.
    * @param szCmd The command to execute.
    * @return EXIT_SUCCESS / EXIT_FAILURE / EXIT_NOT_HANDLED
    */
   uint8_t execute(const char *szCmd, uint8_t nClient) override {
      // validate the call
      if (!szCmd) return EXIT_FAILURE;

      // get the command and arguments into the token buffer
      CxStrToken tkCmd(szCmd, " ");

      // we have a command, find the action to take
      String strCmd = TKTOCHAR(tkCmd, 0);

      // removes heading and trailing white spaces
      strCmd.trim();

      uint8_t nExitValue = EXIT_FAILURE;

      if ((strCmd == "?")) {
         nExitValue = printCommands();
      } else if (strCmd == "disp") {
         String strSubCmd = TKTOCHAR(tkCmd, 1);
         strSubCmd.toLowerCase();

         if (strSubCmd == "enable") {
            _bEnabled = (bool)TKTOINT(tkCmd, 2, 0);
            if (_bEnabled) nExitValue = init();
         } else if (strSubCmd == "init") {
            nExitValue = init();
         } else if (strSubCmd == "on") {
            nExitValue = on();
         } else if (strSubCmd == "off") {
            nExitValue = off();
         } else if (strSubCmd == "clear") {
            nExitValue = clear();
         } else if (strSubCmd == "msg") {
            nExitValue = showMsg(TKTOCHAR(tkCmd, 2));
         } else {
            printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), _bEnabled);
            printf(F(ESC_ATTR_BOLD " Type:         " ESC_ATTR_RESET "%s\n"), getTypeSz());
            printf(F(ESC_ATTR_BOLD " Size:         " ESC_ATTR_RESET "%d x %d\n"), _nWidth, _nHeight);
            printf(F(ESC_ATTR_BOLD " I2C Addr:     " ESC_ATTR_RESET "0x%02X\n"), _nAddr);
            __console.man(getName());
            nExitValue = EXIT_SUCCESS;
         }
      } else {
         // command not handled here
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }

   /**
    * @brief Registers the display as an I2C initialiser.
    */
   void begin() {
      VI2CInitializers.push_back(&_init);
   }

   /**
    * @brief Initialises the Oled display. Called by the i2c capability
    * through the CxInitializer interface or by the command "disp init".
    * @details Creates the display object with the current display type and
    * shows an initialisation message.
    */
   uint8_t init() {
      if (!_bInit) {

         // get the Oled I2C address from the i2c capability, if available
         CxCapabilityI2C* pI2C = CxCapabilityI2C::getInstance();
         if (pI2C && pI2C->getOledAddr() > 0) {
            _nAddr = pI2C->getOledAddr();
         }

         _pDisplay = new Adafruit_SSD1306(_nWidth, _nHeight, &Wire);

         if (_pDisplay == nullptr) {
            __console.error(F("OLED: ### memory allocation failed!"));
            return EXIT_FAILURE;
         }

#ifdef ARDUINO
         if (!_pDisplay->begin(SSD1306_SWITCHCAPVCC, _nAddr)) {
            __console.error(F("OLED: ### init failed on the I2C bus (addr 0x%02X)!"), _nAddr);
            return EXIT_FAILURE;
         }
#endif
         _pDisplay->clearDisplay();
         _pDisplay->setTextSize(1);
         _pDisplay->setTextColor(SSD1306_WHITE);
         _pDisplay->setCursor(0, 0);

         showInitMsg();

         _CONSOLE_INFO(F("OLED: ready, type=%s, size=%dx%d, addr=0x%02X"), getTypeSz(), _nWidth, _nHeight, _nAddr);

         _bInit = true;
         return EXIT_SUCCESS;
      }
      return EXIT_SUCCESS;
   }

   uint8_t end() {
      if (_pDisplay != nullptr) {
#ifdef ARDUINO
         _pDisplay->clearDisplay();
         _pDisplay->display();
         delete _pDisplay;
#endif
         _pDisplay = nullptr;
         _bInit = false;
         return EXIT_SUCCESS;
      }
      return EXIT_FAILURE;
   }

   uint8_t on() {
      if (_pDisplay != nullptr) {
#ifdef ARDUINO
         _pDisplay->display();
#endif
         return EXIT_SUCCESS;
      }
      return EXIT_FAILURE;
   }

   uint8_t off() {
      if (_pDisplay != nullptr) {
#ifdef ARDUINO
         _pDisplay->clearDisplay();
         _pDisplay->display();
#endif
         return EXIT_SUCCESS;
      }
      return EXIT_FAILURE;
   }

   uint8_t clear() {
      if (_pDisplay != nullptr) {
#ifdef ARDUINO
         _pDisplay->clearDisplay();
         _pDisplay->display();
#else
         std::cout << "OLED: (clear)\n";
#endif
         return EXIT_SUCCESS;
      }
      return EXIT_FAILURE;
   }

   /**
    * @brief Shows a message on the display and refreshes it.
    * @param szMsg The message to show.
    */
   uint8_t showMsg(const char* szMsg) {
      if (_pDisplay == nullptr) return EXIT_FAILURE;
      if (szMsg == nullptr) return EXIT_FAILURE;

      _pDisplay->clearDisplay();
      _pDisplay->setCursor(0, 0);
#ifdef ARDUINO
      _pDisplay->print(szMsg);
      _pDisplay->display();
#else
      _pDisplay->println(szMsg);
#endif
      return EXIT_SUCCESS;
   }

   /**
    * @brief Shows the initialisation message on the display.
    */
   void showInitMsg() {
      if (_pDisplay == nullptr) return;

      _pDisplay->clearDisplay();
      _pDisplay->setCursor(0, 0);
#ifdef ARDUINO
      _pDisplay->println(F("ESP Console Plus"));
      _pDisplay->println(F("Display init"));
      _pDisplay->display();
#else
      _pDisplay->println("ESP Console Plus");
      _pDisplay->println("Display init");
#endif
   }

   /**
    * @brief Sets the display type.
    * @param eType The display type.
    */
   void setType(EOLEDType eType) {
      _eType = eType;
      switch (_eType) {
         case EOLEDType::oled128x32:
            _nWidth = 128;
            _nHeight = 32;
            break;
         default:
            break;
      }
   }

   EOLEDType getType() { return _eType; }

   /**
    * @brief Gets the display type as a string.
    */
   const char* getTypeSz() {
      switch (_eType) {
         case EOLEDType::oled128x32: return "oled128x32";
         default: return "none";
      }
   }

   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   bool isInit() {return _bInit;}
};

/**
 * @brief Initialises the display through the i2c capability.
 * @details Called through the CxInitializer interface when the i2c
 * capability is initialised.
 */
void CxDisplayInit::init() {
   if (_pDisplay) _pDisplay->init();
}

#endif /* CxCapabilityDisplay_hpp */
