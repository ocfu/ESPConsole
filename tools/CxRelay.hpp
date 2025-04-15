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
   
   static void _rlyAction(CxDevice* dev, uint8_t id, const char* cmd) {
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
      if (__bPersistent) {
         _timerOff.setPeriod(__console.loadSettingInt("ot", _timerOff.getPeriod(), getName()));
         setDefaultOn(__console.loadSettingInt("df", isDefaultOn(), getName()));
      }

      if (isDefaultOn()) {
         on();
      } else {
         off();
      }
   }
   
   virtual void loop(bool bDegraded = false) override {
      _timerOff.loop();
   }

   virtual const std::vector<String> getHeadLine(bool bDefault = true) override {
      if (bDefault) {
         return CxGPIODevice::getHeadLine();
      } else {
         std::vector<String> headLine = CxGPIODevice::getHeadLine();
         headLine.push_back(F("Off-timer"));
         headLine.push_back(F("Default-on"));
         return headLine;
      }
   };
   
   virtual const std::vector<uint8_t> getWidths(bool bDefault = true) override {
      if (bDefault) {
         return CxGPIODevice::getWidths();
      } else {
         std::vector<uint8_t> widths = CxGPIODevice::getWidths();
         widths.push_back(10); // off-timer
         widths.push_back(10); // default-on
         return widths;
      }
   };
   
   virtual const std::vector<String> getData(bool bDefault = true) override {
      if (bDefault) {
         return CxGPIODevice::getData();
      } else {
         std::vector<String> data = CxGPIODevice::getData();
         data.push_back(String(_timerOff.getPeriod()));
         data.push_back(isDefaultOn() ? "yes" : "no");
         return data;
      }
   }

   void setPin(uint8_t nPin) {CxGPIODevice::setPin(nPin); setPinMode(OUTPUT);} // overwrite virtual base function. RELAY is always an output

   void setDefaultOn(bool set = true) {
      if (set != m_bDefaultOn && __bPersistent) {
         __console.saveSettingInt("df", set, "", getName());
      }
      m_bDefaultOn = set;
   }
   bool isDefaultOn() {return m_bDefaultOn;}
   
   void toggle() {if (isOn()) off(); else on();}
   void on() {
      if (!isEnabled()) {
         return;
      }
      if (!isHigh()) {
         setHigh();
         callCb(ERelayEvent::relayon);
         _CONSOLE_INFO(F("RLY: Relay on GPIO%02d switched on"), getPin());
         if (_timerOff.getPeriod() > 0) {
            _CONSOLE_INFO(F("RLY: Relay on GPIO%02d start off-timer (%dms)"), getPin(), _timerOff.getPeriod());
            __console.processCmd("led blink");
            _timerOff.start([this](){
               _CONSOLE_INFO(F("RLY: Relay on GPIO%02d off-timer ends"));
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
         callCb(ERelayEvent::relayoff);
         _CONSOLE_INFO(F("RLY: Relay on GPIO%02d switched off"), getPin());
         __console.processCmd("led off");
      }
   }
   bool isOn() {return isHigh();}
   bool isOff() {return isLow();}
      

   uint32_t getOffTimer() {return _timerOff.getPeriod();}
   void setOffTimer(uint32_t nTime) {
      if (nTime != _timerOff.getPeriod()) {
         _CONSOLE_INFO(F("RLY: Relay on GPIO%02d set off-timer to %dms"), getPin(), nTime);
         _timerOff.setPeriod(nTime);
         if (__bPersistent) __console.saveSettingInt("ot", nTime, "", getName());
      } else {
         _timerOff.restart();
      }
   }

};


#endif /* CxRelay_hpp */
