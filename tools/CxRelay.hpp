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
   
private:
   uint8_t _nId = 0;

   CxTimer _timerOff;
   
   bool _bEnabled = true;
   bool m_bDefaultOn = false;
   
   static void _rlyAction(CxGPIODevice* dev, uint8_t id, const char* cmd) {
      ESPConsole.processCmd(cmd);
   }
   
public:
   CxRelay() : CxRelay(-1) {}
   CxRelay(uint8_t nPin = -1, const char* name = "", bool bInverted = false, const char* cmd = "", cbFunc fp = nullptr) : CxGPIODevice(nPin, OUTPUT, bInverted, cmd) {addCallback(_rlyAction);addCallback(fp);setName(name);}

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
         __console.printf(F(ESC_ATTR_BOLD "| Off-timer | Default-on | Command" ESC_ATTR_RESET ));
      }
   }
   
   virtual void printData(bool bGeneral = true) override {
      if (bGeneral) {
         CxGPIODevice::printData();
      } else {
         __console.printf(F("| %9d | %10s | %s"), getOffTimer(), isDefaultOn() ? "on" : "off", getCmd());
      }
   }
   
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
         callCb(ERelayEvent::relayon);
         __console.info(F("RLY: Relay on GPIO%02d switched on"), getPin());
         if (_timerOff.getPeriod() > 0) {
            __console.info(F("RLY: Relay on GPIO%02d start off-timer (%dms)"), getPin(), _timerOff.getPeriod());
            __console.processCmd("led blink");
            _timerOff.start([this](){
               __console.info(F("RLY: Relay on GPIO%02d off-timer ends"));
               off();
               __console.processCmd("led off");
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
         callCb(ERelayEvent::relayoff);
         __console.info(F("RLY: Relay on GPIO%02d switched off"), getPin());
      }
   }
   bool isOn() {return isHigh();}
   bool isOff() {return isLow();}
      

   uint32_t getOffTimer() {return _timerOff.getPeriod();}
   void setOffTimer(uint32_t nTime) {
      __console.info(F("RLY: Relay on GPIO%02d set off-timer to %dms"), getPin(), nTime);
      _timerOff.setPeriod(nTime);
   }

};


#endif /* CxRelay_hpp */
