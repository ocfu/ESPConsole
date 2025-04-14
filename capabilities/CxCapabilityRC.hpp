
#ifndef CxCapabilityRC_hpp
#define CxCapabilityRC_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityFS.hpp"

#include "../tools/CxGpioTracker.hpp"
#include "../tools/CxTimer.hpp"

// Info: http://www.rflink.nl/blog2/wiring
// https://github.com/sui77/rc-switch


#ifdef ARDUINO
#include <RCSwitch.h>
#else
#define RCSwitch int
#include <iostream>
#endif

#define RCCHANNELS 4


#include <map>

#ifndef ARDUINO
#endif

class CxCapabilityRC : public CxCapability {
   bool _bEnabled = false;
   
   CxTimer1s _timerUpdate;

   RCSwitch *m_pRCSwitch = NULL;
   
   CxGPIO m_gpioRx;
   CxGPIO m_gpioTx;
   
   struct {
      bool isOn;
      bool isToggle;
      unsigned long nLast;
      unsigned long nOnCode;
      unsigned long nOffCode;
   } m_aCh[RCCHANNELS];
   
   uint8_t _nRepeatTransmit = 4;

   String _strFriendlyName;
   
   CxGPIODeviceManagerManager& _gpioDeviceManager = CxGPIODeviceManagerManager::getInstance();

   // call back

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
    * @brief Default constructor.
    * @details Initializes the segment display capability with the "seg" command.
    */
   explicit CxCapabilityRC()
   : CxCapability("rc", getCmds()) {}
   /**
    * @brief Gets the name of the segment display capability.
    * @return The name of the segment display capability.
    */
   static constexpr const char* getName() { return "rc"; }
   /**
    * @brief Gets the commands supported by the segment display capability.
    * @return A vector of commands supported by the segment display capability.
    */
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "rc" };
      return commands;
   }
   /**
    * @brief Constructs a segment display capability with the given parameters.
    * @param param The parameters for the segment display capability.
    * @return A unique pointer to the constructed segment display capability.
    * @details This method constructs a segment display capability with the given parameters
    */
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityRC>();
   }

   /**
    * @brief Destructor.
    * @details Stops the segment display capability and clears the screen buffer.
    *
    */
   ~CxCapabilityRC() {
      end();
      _bEnabled = false;
   }
   
   void setRxEvent(void (*p)(int nCh, bool bOn)) {}

   
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
      
      if (__console.isSafeMode()) {
         __console.error(F("Safe mode active!"));
         return;
      }
     
      __console.executeBatch("init", getName());

   }
   
   static CxCapabilityRC* getInstance() {
      return static_cast<CxCapabilityRC*>(ESPConsole.getCapInstance("rf"));
   }

   /**
    * @brief Loops the segment display capability.
    * @details Updates the segment display content and handles display updates.
    * The method also handles screen transitions and slide show functionality.
    */
   void loop() override {
      if (_bEnabled) {
         unsigned long now = millis();
#ifdef ARDUINO
         if ((m_pRCSwitch != NULL) && m_pRCSwitch->available()) {
            
            unsigned long nValue = m_pRCSwitch->getReceivedValue();
            
            for (int i = 0; i < RCCHANNELS; i++) {
               int nSwitchState = -1; //-1=no switch, 0=switch off, 1=switch on               
                
               if (isToggle(i) && (nValue == m_aCh[i].nOnCode || nValue == m_aCh[i].nOffCode)) {
                  // debounce
                  if ((now - m_aCh[i].nLast) > 500) {
                     nSwitchState = m_aCh[i].isOn ? 0 : 1;
                  }
               } else if (nValue == m_aCh[i].nOnCode) {
                  nSwitchState = 1;
               } else if (nValue == m_aCh[i].nOffCode) {
                  nSwitchState = 0;
               }
               
               switch (nSwitchState) {
                  case 0:
                     m_aCh[i].isOn = false;
                     m_aCh[i].nLast = now;
                     
                     // callback
//                     if(cbRxEvent != nullptr) {
//                        cbRxEvent(i, false);
//                     }
                     break;
                  case 1:
                     m_aCh[i].isOn = true;
                     m_aCh[i].nLast = now;
                     // callback
//                     if(cbRxEvent != nullptr) {
//                        cbRxEvent(i, true);
//                     }
                     break;
                  default:
                     break;
               }
            }
            m_pRCSwitch->resetAvailable();
         }
#endif
      }
   }

   /**
    * @brief Initializes the segment display capability.
    * @details Sets up the segment display capability and initializes the display object.
    * The method also loads the segment display screens and sets the brightness level.
    * @return True if the initialization is successful, otherwise false.
    *
    */
   bool execute(const char *szCmd) override {
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
      
      // expect sz parameter, invalid is nullptr
      const char* a = TKTOCHAR(tkCmd, 1);
      const char* b = TKTOCHAR(tkCmd, 2);
      
      if ((strCmd == "?")) {
         printCommands();
      } else if (strCmd == "rc") {
         String strSubCmd = TKTOCHAR(tkCmd, 1);
         if (strSubCmd == "enable") {
            _bEnabled = (bool)TKTOINT(tkCmd, 2, 0);
            if (_bEnabled) init();
         } else if (strSubCmd == "list") {
         } else if (strSubCmd == "test") {
            test();
         } else if (strSubCmd == "on") {
            on(TKTOINT(tkCmd, 2, 0));
         } else if (strSubCmd == "off") {
            off(TKTOINT(tkCmd, 2, 0));
         } else if (strSubCmd == "setpins" && (tkCmd.count() >= 3)) {
            setPins(TKTOINT(tkCmd, 2, -1), TKTOINT(tkCmd, 3, -1));
         } else if (strSubCmd == "fn") {
            _strFriendlyName = TKTOCHAR(tkCmd, 2);
         } else if (strSubCmd == "ch") {
            uint8_t ch = TKTOINT(tkCmd, 2, 0);
            setOnCode(ch, TKTOINT(tkCmd, 3, 0));
            setOffCode(ch, TKTOINT(tkCmd, 4, 0));
            setToggle(ch, TKTOINT(tkCmd, 5, 0));
         } else if (strSubCmd == "init") {
            init();
         } else if (strSubCmd == "let") {
            String strOpertor = TKTOCHAR(tkCmd, 3);
            CxGPIODevice* dev1 = _gpioDeviceManager.getDevice(TKTOCHAR(tkCmd, 2));
            CxGPIODevice* dev2 = _gpioDeviceManager.getDevice(TKTOCHAR(tkCmd, 4));
            
            if (dev1 && dev2) {
               if (strOpertor == "=") {
                  dev1->set(dev2->get());
               }
            } else if (dev2) {
               uint8_t ch = TKTOINT(tkCmd, 2, INVALID_UINT8);
               
               if (ch != INVALID_UINT8) {
                  if (dev2->get()) {
                     on(ch);
                  } else {
                     off(ch);
                  }
               }
            }
            else {
               println(F("device not found!"));
            }
         } else if (strSubCmd == "repeat") {
            _nRepeatTransmit = TKTOINT(tkCmd, 2, _nRepeatTransmit);
         }
         else {
            printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), _bEnabled);
            println(F("rf commands:"));
            println(F("  enable 0|1"));
            println(F("  setpins <rx> <tx>"));
            println(F("  list"));
            println(F("  on <ch>"));
            println(F("  off <ch>"));
            println(F("  fn <name>"));
            println(F("  ch <channel> <on-code> <off-code> <toggle>"));
            println(F("  test"));
            println(F("  init"));
            println(F("  repeat <n>"));
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

         // at least one pin must be set, otherwise disable the service
         if ((m_gpioRx.isValid() || m_gpioTx.isValid()) && (m_gpioRx.getPin() != m_gpioTx.getPin())) {
            
            _CONSOLE_INFO(F("RC: start rf..."));
            if (Led1.getPin() == m_gpioRx.getPin() || Led1.getPin() == m_gpioTx.getPin()) {
               _CONSOLE_INFO(F("RC: disable Led1, use of same gpio %d."), Led1.getPin());
               Led1.setPin(-1);
            }
            
            _CONSOLE_INFO(F("RC: start service..."));
            if (m_pRCSwitch != NULL) {
               delete m_pRCSwitch;
            }
            for (int i = 0; i < RCCHANNELS; i++) {
               m_aCh[i].isOn = false;
               m_aCh[i].nLast = 0;
            }
            m_pRCSwitch = new RCSwitch();
            if (m_pRCSwitch != NULL) {
#ifdef ARDUINO
               if ( m_gpioTx.getPin() >= 0) {
                  m_pRCSwitch->enableTransmit(m_gpioTx.getPin());
                  m_pRCSwitch->setRepeatTransmit(4);
               }
               
               if ( m_gpioRx.getPin() >= 0) {
                  m_pRCSwitch->enableReceive(m_gpioRx.getPin());
               }
#endif
            } else {
               __console.error(F("RC: ### cannot initialize RCSwitch"));
               return false;
            }
         }
      }
      return false;
   }
   
   bool init(int nPinRx, int nPinTx) {
      setPins(nPinRx, nPinTx);
      return init();
   }

   void end() {
   }
   
   bool hasValidPins() {return (m_gpioRx.isValid() && m_gpioTx.isValid() && m_gpioRx.getPin() != m_gpioTx.getPin());}
   
   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   void setPins(int nPinRx, int nPinTx) { ///< Set the rx and tx pins for the segment display
      m_gpioRx.setPin(nPinRx);
      m_gpioRx.setPinMode(INPUT);
      m_gpioRx.setName("rx");
      m_gpioTx.setPin(nPinTx);
      m_gpioTx.setPinMode(OUTPUT);
      m_gpioTx.setName("tx");
   }
   
   CxGPIO& getGPIOTx() {return m_gpioTx;}
   CxGPIO& getGPIORx() {return m_gpioRx;}

   void setOnCode(int iCh, unsigned long nCode) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         m_aCh[iCh].nOnCode = nCode;
      }
   }
   
   unsigned long getOnCode(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         return m_aCh[iCh].nOnCode;
      }
      return 0;
   }
   
   void setOffCode(int iCh, unsigned long nCode){
      if (iCh >= 0 && iCh < RCCHANNELS) {
         m_aCh[iCh].nOffCode = nCode;
      }
   }
   
   unsigned long getOffCode(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         return m_aCh[iCh].nOffCode;
      }
      return 0;
   }
   
   void setToggle(int iCh, bool set) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         m_aCh[iCh].isToggle = set;
      }
   }
   
   bool isToggle(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         return m_aCh[iCh].isToggle;
      }
      return false;
   }

   bool on(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS && (m_pRCSwitch != NULL) && getOnCode(iCh) > 0) {
         _CONSOLE_INFO(F("RC: switch (%d) on (code = %lu)"), iCh, getOnCode(iCh));
#ifdef ARDUINO
         m_pRCSwitch->send(getOnCode(iCh), 24);
#endif
         setOnState(iCh, true);
         return true;
      }
      return false;
   }

   bool off(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS && (m_pRCSwitch != NULL) && getOffCode(iCh) > 0) {
         _CONSOLE_INFO(F("RC: switch (%d) off (code = %lu)"), iCh, getOffCode(iCh));
#ifdef ARDUINO
         m_pRCSwitch->send(getOffCode(iCh), 24);
#endif
         setOnState(iCh, false);
         return true;
      }
      return false;
   }
   
   bool toggle(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         if (isOn(iCh)) {
            off(iCh);
            return true;
         } else {
            on(iCh);
            return true;
         }
      }
      return false;
   }
   
   void setOnState(int iCh, bool set) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         m_aCh[iCh].isOn = set;
         m_aCh[iCh].nLast = millis();
      }
   }
   
   bool isOn(int iCh) {
      if (iCh >= 0 && iCh < RCCHANNELS) {
         return m_aCh[iCh].isOn;
      }
      return false;
   }



   /**
    * @brief Tests the segment display capability.
    * @details Tests the segment display capability by displaying various numbers, messages, and symbols on the segment display.
    * The method displays numbers from -110 to 100, the time, the save message, the error message, the on message, and the off message.
    */
   void test() {
   }
    
    
   static void loadCap() {
      CAPREG(CxCapabilityRC);
      CAPLOAD(CxCapabilityRC);
   };

};


#endif /* CxCapabilityRC_hpp */

