//
//  CxLed.hpp
//  test
//
//  Created by ocfu on 30.07.22.
//

#ifndef CxLed_hpp
#define CxLed_hpp

#include "CxGpioDeviceManager.hpp"
#include "CxTimer.hpp"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

class CxLed : public CxGPIODevice {
   CxTimer _timer;
   
   uint8_t  _nFlashCnt = 0;
   uint32_t _nDutyTime = 0;
   
   
public:
   CxLed(int nPin, const char* name = "", bool bInverted = false, const char* cmd = "", cbFunc fp = nullptr) : CxGPIODevice(nPin, OUTPUT, bInverted, cmd) {addCallback(fp);setName(name);}
   
   virtual const char* getTypeSz() override {return "led";}
   
   void setPin(uint8_t nPin) {CxGPIODevice::setPin(nPin); setPinMode(OUTPUT);} // overwrite virtual base function. RELAY is always an output


   uint8_t getFlashCnt() {return _nFlashCnt;}
   uint32_t getPeriod() {return _timer.getPeriod();}
   uint32_t getDutyTime() {return _nDutyTime;}
   uint32_t getElapsedTime() {return _timer.getElapsedTime();}

   void on(bool bOn) {bOn ? on():off();}
   void on() {setBlink(0); setHigh();}
   void off() {setBlink(0); setLow();}
   bool isOn() {return isHigh();}
   bool isOff() {return isLow();}
   

   void setBlink(uint32_t period = 1000, uint8_t duty = 128) {_nFlashCnt = 0; _timer.start(period); _nDutyTime = (uint32_t)((period * duty)/255);}
   void setFlash(uint32_t period = 250, uint8_t duty = 128, uint8_t cnt = 1) {_nFlashCnt = cnt; _timer.start(period); _nDutyTime = (uint32_t)((period * duty)/255);}
   
   void action() {
      if (!isValid()) return;
      if (_timer.isRunning()) {
         if (isHigh() && _timer.getElapsedTime() >= _nDutyTime) {
            // Turn off the LED after duty time has elapsed
            setLow();
            _timer.restart();
            if (_nFlashCnt) {
               _nFlashCnt--;
               if (!_nFlashCnt) off();
            }
         } else if (isLow() && _timer.getElapsedTime() >= (_timer.getPeriod() - _nDutyTime)) {
            // Turn on the LED after the OFF duration has elapsed
            setHigh();
            _timer.restart();
         }
      }
   }
   
   bool isBlinking() {return _timer.isRunning();}
   
   void blinkOk() {setBlink(1000);}
   void blinkError() {setBlink(500);}
   void blinkBusy() {setBlink(1000, 1);}
   void blinkFlash() {setBlink(200, 0.01*255);}
   void blinkData() {setBlink(75, 0.01*255);}
   void blinkWait() {setBlink(2000, 20);}
   void blinkConnect() {setBlink(2000, 0.9*255);}

   void flashOk() {setFlash(1000, 128, 2);}
   void flashError() {setFlash(500, 128, 3);}
   void flashBusy() {setFlash(1000, 1, 3);}
   void flashFlash() {setFlash(200, 0.01*255, 3);}
   void flashData() {setFlash(75, 0.01*255, 3);}
   void flashWait() {setFlash(2000, 20, 3);}
   void flashConnect() {setFlash(2000, 0.9*255, 3);}

   void flash() {setFlash(100, 255, 1);}
   void flashNr(uint8_t cnt) {setFlash(1000, 0.1*255, cnt);}   
   
};

#endif /* CxLed_hpp */
