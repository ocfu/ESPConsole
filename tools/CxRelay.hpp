//
//  CxRelay.hpp
//  test
//
//  Created by ocfu on 31.07.22.
//

#ifndef CxRelay_hpp
#define CxRelay_hpp

#include "CxGpioDeviceManager.hpp"
#include "CxSensorManager.hpp"
#include "CxTimer.hpp"

class CxRelay : public CxGPIODevice {
public:
   enum ERelayEvent {relayon, relayoff};
   
public:
   typedef void (*cb_t)(CxRelay::ERelayEvent);

private:
   uint8_t _nId = 0;

   CxTimer _timerOff;
   
   bool _bEnabled = true;
   bool m_bDefaultOn = false;
   
protected:
   // callback for button event
   // TODO: make it a function pointer
   cb_t __cb = nullptr;
   

public:
   CxRelay() : CxRelay(-1) {}
   CxRelay(uint8_t nPin = -1, const char* name = "", bool bInverted = false, const char* cmd = "", cb_t fp = nullptr) : CxGPIODevice(nPin, OUTPUT, bInverted, cmd) {__cb = fp;setName(name);}

   virtual ~CxRelay() {}

   virtual const char* getTypeSz() override {return "relay";}
   
   void setId(uint8_t idj) {_nId = idj;}
   uint8_t getId() {return _nId;}

   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   virtual void begin() override {
      if (isDefaultOn()) {
         on();
      } else {
         off();
      }
   }
   
   virtual void loop(bool bDegraded = false) override {
      _timerOff.loop();
   }

   virtual void printHeadLine(bool bGeneral = true) override {
      if (bGeneral) {
         CxGPIODevice::printHeadLine();
      } else {
         __console.printf(F(ESC_ATTR_BOLD "| Off-timer | Default-on " ESC_ATTR_RESET ));
      }
   }
   
   virtual void printData(bool bGeneral = true) override {
      if (bGeneral) {
         CxGPIODevice::printData();
      } else {
         __console.printf(F("| %9d | %10s "), getOffTime(), isDefaultOn() ? "on" : "off");
      }
   }

   void setCallback(cb_t fp) {__cb = fp;}
   
   void setPin(uint8_t nPin) {CxGPIODevice::setPin(nPin); setPinMode(OUTPUT);} // overwrite virtual base function. RELAY is always an output

   void setDefaultOn(bool set = true) {m_bDefaultOn = set;}
   bool isDefaultOn() {return m_bDefaultOn;}
   
   void toggle() {if (isOn()) off(); else on();}
   void on() {
      if (!isEnabled()) {
         return;
      }
      if (!isHigh()) {
         setHigh();
         if (__cb) __cb(ERelayEvent::relayon);
         __console.info(F("RLY: Relay on GPIO%02d switched on"), getPin());
         if (_timerOff.getPeriod() > 0) {
            __console.info(F("RLY: Relay on GPIO%02d start off-timer (%dms)"), getPin(), _timerOff.getPeriod());
            _timerOff.start([this](){
               __console.info(F("RLY: Relay on GPIO%02d off-timer ends"));
               off();
            }, true); // stop after due
         }
      }
   }
   
   void off() {
      if (!isEnabled()) {
         return;
      }
      if (!isLow()) {
         setLow();
         if (__cb) __cb(ERelayEvent::relayoff);
         __console.info(F("RLY: Relay on GPIO%02d switched off"), getPin());
      }
   }
   bool isOn() {return isHigh();}
   bool isOff() {return isLow();}
      

   uint32_t getOffTime() {return _timerOff.getPeriod();}
   void setOffTime(uint32_t nTime) {
      __console.info(F("RLY: Relay on GPIO%02d set off-timer to %dms"), getPin(), nTime);
      _timerOff.setPeriod(nTime);
   }

};


#endif /* CxRelay_hpp */
