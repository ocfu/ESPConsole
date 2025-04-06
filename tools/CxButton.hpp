//
//  CxButton.hpp
//  test
//
//  Created by ocfu on 31.07.22.
//

#ifndef CxButton_hpp
#define CxButton_hpp

#include "CxGpioDeviceManager.hpp"
#include "CxTimer.hpp"

class CxButton : public CxGPIODevice {
   
public:
   enum EBtnEvent {pressed, singlepress, pressed10s, reset, doublepress, multiplepress, cleared};
   
   
private:
   uint8_t _nId = 0;
   int _nState = 0;
   bool _bRebootButton = false;

   CxTimer _timer;
   
   bool _bEnabled = true;
   CxLed* _pLed = nullptr;
   
   const uint32_t _nLongPressTime = 10000; // ms
   const uint32_t _nShortPressTime = 250; // ms
   const uint32_t _nIdleTime = 2000; // ms
   const uint32_t _nDebounceTime = 100; // ms
         
   
   static void _btnAction(CxGPIODevice* dev, uint8_t id, const char* cmd) {
      String strCmd;
      strCmd.reserve((uint32_t)(strlen(cmd) + 10)); // preserve some space for the command and additional parameters.
      strCmd = cmd;
      
      if (id == CxButton::EBtnEvent::singlepress) {
         ESPConsole.processCmd(cmd);
      } else if (id == CxButton::EBtnEvent::doublepress) {
         strCmd += " #double";
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxButton::EBtnEvent::multiplepress) {
         strCmd += " #multi";
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxButton::EBtnEvent::pressed10s) {
         strCmd += " #long";
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxButton::EBtnEvent::reset) {
         strCmd += " #reset";
         ESPConsole.processCmd(strCmd.c_str());
      }
   };

public:
   CxButton(uint8_t nPin = -1, const char* name = "", bool bInverted = false, const char* cmd = "", cbFunc fp = nullptr) : CxGPIODevice(nPin, INPUT, bInverted, cmd) {addCallback(_btnAction); addCallback(fp);setName(name);}
   //CxButton(uint8_t nPin = -1, bool bInverted = false, const char* name = "", isr_t isr = nullptr) : CxGPIODevice(nPin, isr) {setName(name);setInverted(bInverted);}

   virtual ~CxButton() {end();}

   virtual void begin() override {
      if (__isrId >= 0) {
         enableISR();
      }
   };
   
   virtual void end() override {
      if (__isrId >= 0) {
         disableISR();
      }
   };
      
   virtual const char* getTypeSz() override {return "button";}
   
   bool isResetButton() {return _bRebootButton;}
   void setResetButton(bool set) {_bRebootButton = set;}
   
   void setId(uint8_t idj) {_nId = idj;}
   uint8_t getId() {return _nId;}

   void setLed(CxLed* led) {_pLed = led;}
  // void setRelay(CxRelay* relay) {m_pRelay = relay;}
   
   bool isPressed() {return isHigh();}
   
   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   virtual void loop(bool bDegraded = false) override {  // degraded: e.g. in Wifi in AP mode or esp in setup process. If degraded, then no callback and no relay action
      
      // FIXME: works only one button at a time
      static uint8_t cnt = 0; // counts number of pressed state in short time (2s)
      
      if (!isEnabled()) {
         return;
      }
      
      switch (_nState) {
         case 0: // default state
            if (isHigh()) {
               cnt++;
               _nState = 1;
               _timer.start(_nLongPressTime, false); // set timer for long time pressed
               if (_pLed != nullptr) _pLed->on();
               if (!bDegraded) callCb(CxButton::EBtnEvent::pressed, getCmd());
               _CONSOLE_DEBUG_EXT(DEBUG_FLAG_GPIO, F("BTTN: Button on GPIO%02d was pressed! (%dx)"), getPin(), cnt);
            }
            break;
            
         case 1: // button pressed state
            if (isHigh()) {
               // do nothing untill button is released...or long time pressed
               if (_timer.isDue()) {
                  if (!bDegraded) {
                     callCb(CxButton::EBtnEvent::pressed10s, getCmd());
                  } else if (isResetButton()) {
                     if(_pLed != nullptr) _pLed->blinkBusy();
                  }
                  _nState = 4; // state for long time pressed button
                  _CONSOLE_DEBUG_EXT(DEBUG_FLAG_GPIO, F("BTTN: Button on GPIO%02d in long pressed state now!"), getPin());
               }
               // remain in state 1 while debouncing
            } else if (_timer.getElapsedTime() > _nDebounceTime) { // ensures that button state is not high for 10ms
               // button was released
               _nState = 2;
               _timer.start(_nShortPressTime, false); // give time to detect multiple pressings
            }
            break;
         case 2: // button released state
            // detect multiple pressings
            if (_timer.getElapsedTime() > _nDebounceTime) {
               if (isHigh()) {
                  _nState = 0;
                  break;
               }
            }
            if (_timer.isDue()) {
               if (cnt == 1) {
                  if (!bDegraded) {
                     _CONSOLE_DEBUG_EXT(DEBUG_FLAG_GPIO, F("BTTN: Button on GPIO%02d was single pressed!"), getPin());
                     callCb(CxButton::EBtnEvent::singlepress, getCmd());
                  }
                  /*
                  if (m_pRelay != nullptr && !bDegraded) {
                     m_pRelay->toggle();
                     if (m_pLed != nullptr) {
                        if (m_pRelay->isOn()) {
                           m_pLed->on();
                        } else {
                           m_pLed->off();
                        }
                     }
                  }
                  */
               } else {
                  if (!bDegraded) {
                     _CONSOLE_DEBUG_EXT(DEBUG_FLAG_GPIO, F("BTTN: Button on GPIO%02d was pressed %dx!"), getPin(), cnt);
                     if (cnt == 2) {
                        callCb(CxButton::EBtnEvent::doublepress, getCmd());
                     } else {
                        callCb(CxButton::EBtnEvent::multiplepress, getCmd());
                     }
                  }
               }
               _nState = 3;
               _timer.start(_nIdleTime, false); // set idle state
               break;
            }
         case 3: // idle state. useful to indicate button state in home assistant for a while
            if (_timer.isDue()) {
               if (!bDegraded) {
                  callCb(CxButton::EBtnEvent::cleared, getCmd());
               }
               _nState = 0;
               cnt = 0;
               if(_pLed != nullptr) _pLed->off();
               _CONSOLE_DEBUG_EXT(DEBUG_FLAG_GPIO, F("BTTN: Button on GPIO%02d was cleared!"), getPin());
            } else {
               // wakeup, if button is pressed again and count it
               if (isHigh()) {
                  _nState = 0;
               }
            }
            break;
         case 4: // the 10+s case
            if (isHigh()) {
               // still pressed, do nothing
            } else { // button finally released after 10+s
               // factory reset after 10+ seconds
               _CONSOLE_DEBUG_EXT(DEBUG_FLAG_GPIO, F("BTTN: Button on GPIO%02d was long pressed!"), getPin());
               if (!bDegraded) {
                  callCb(CxButton::EBtnEvent::reset, getCmd());
               } else if (isResetButton()) {
                  if(_pLed != nullptr) _pLed->off();
                  //::factoryReset();
                  __console.processCmd("reboot -f");
               }
               _nState = 0;
               cnt = 0;
            }
            break;
      }
   }
};

class CxButtonReset : public CxButton {
public:
   CxButtonReset(int nPin = -1, const char* name = "", bool bInverted = false, cbFunc fp = nullptr) : CxButton(nPin, name, bInverted, "reset", fp) {setPin(nPin); setPinMode(INPUT); setResetButton(true);}
   
   const char* getTypeSz() {return "reset";}
   
};

#endif /* CxButton_hpp */
