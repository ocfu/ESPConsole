//
//  CxAnalog.hpp
//
//
//  Created by ocfu on 27.04.25.
//

#ifndef CxAnalog_hpp
#define CxAnalog_hpp

#include "CxGpioDeviceManager.hpp"
#include "CxTimer.hpp"

class CxAnalog : public CxGPIODevice {
   
public:
   enum EAnalogEvent {value, raiseabove, raisebelow};
   
   
private:
   uint8_t  _nId = 0;
   uint8_t  _nState = 0;
   
   int16_t _nValue = 0;
   
   CxTimer _timer;
   
   bool _bEnabled = true;
   
   static void _Action(CxDevice* dev, uint8_t id, const char* cmd) {
      if (!cmd  || !*cmd) return;
      
      String strCmd;
      strCmd.reserve((uint32_t)(strlen(cmd) + 10)); // preserve some space for the command and additional parameters.
      strCmd = cmd;
      
      if (id == CxAnalog::EAnalogEvent::raiseabove) {
         strCmd += " #above";
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxAnalog::EAnalogEvent::raisebelow) {
         strCmd += " #below";
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxAnalog::EAnalogEvent::value) {
         strCmd.replace(F("$VALUE"), String(dev->get()));
         ESPConsole.processCmd(strCmd.c_str());
      }
   };
   
public:
   CxAnalog(uint8_t nPin = -1, const char* name = "", bool bInverted = false, const char* cmd = "", cbFunc fp = nullptr) : CxGPIODevice(nPin, INPUT, bInverted, cmd) { addCallback(fp); addCallback(_Action);setName(name);_timer.start(100, false);}
   
   virtual ~CxAnalog() {end();}
   
   virtual const char* getTypeSz() override {return "analog";}
   
   void setId(uint8_t idj) {_nId = idj;}
   uint8_t getId() {return _nId;}
   
   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   void setTimer(uint32_t set) { if (set > 100) _timer.start(set); else _timer.start(100, false);}
   
   virtual void loop(bool bDegraded = false) override {  // degraded: e.g. in Wifi in AP mode or esp in setup process. If degraded, then no callback and no relay action
      
      if (!isEnabled()) {
         return;
      }
      
      // don't call analog read at every cylce, it will cause wifi connections problems!
      // If the sensor read and processing takes less than 1 ms, the loop may return and
      // you will get multiple executions of analogRead within the same millisecond - not the desired result.
      // Considering that analogRead(A0) itself only takes about 70 microseconds,
      // if the processing is quick you could easily have 10 or so executions of analogRead(A0) in that millisecond
      
      if (_timer.isDue()) {
         _nValue = get();
         ESPConsole.addVariable(getName(), _nValue);
         callCb(CxAnalog::EAnalogEvent::value);
      }
      
   }
};

#endif /* CxAnalog_hpp */
