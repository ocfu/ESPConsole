//
//  CxContact.hpp
//
//
//  Created by ocfu on 27.04.25.
//

#ifndef CxContact_hpp
#define CxContact_hpp

#include "CxGpioDeviceManager.hpp"
#include "CxTimer.hpp"

class CxContact : public CxGPIODevice {
   
public:
   enum EContactEvent {open, close};
   
   
private:
   uint8_t _nId = 0;
   uint8_t _nState = 0;

   CxTimer _timer;
   
   bool _bEnabled = true;
   
   static void _Action(CxDevice* dev, uint8_t id, const char* cmd) {
      String strCmd;
      strCmd.reserve((uint32_t)(strlen(cmd) + 10)); // preserve some space for the command and additional parameters.
      strCmd = cmd;
      
      if (id == CxContact::EContactEvent::open) {
         strCmd.replace(F("$STATE"), "0");
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxContact::EContactEvent::close) {
         strCmd.replace(F("$STATE"), "1");
         ESPConsole.processCmd(strCmd.c_str());
      }
   };

public:
   CxContact(uint8_t nPin = -1, const char* name = "", bool bInverted = false, bool bPullup = false, const char* cmd = "", cbFunc fp = nullptr) : CxGPIODevice(nPin, bPullup ? INPUT_PULLUP: INPUT, bInverted, cmd) { addCallback(fp ? fp : _Action);setName(name);}

   virtual ~CxContact() {end();}

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
      
   virtual const char* getTypeSz() override {return "contact";}
   
   void setId(uint8_t idj) {_nId = idj;}
   uint8_t getId() {return _nId;}

   bool isClosed() {return isHigh();}
   
   void setEnabled(bool set) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   virtual void loop(bool bDegraded = false) override {  // degraded: e.g. in Wifi in AP mode or esp in setup process. If degraded, then no callback and no relay action
      
      if (!isEnabled()) {
         return;
      }
      
      switch (_nState) {
         case 0: // default state (contact is open)
            if (isHigh()) {
               _nState = 1;
               _timer.start(getDebounce()); // set timer for debounce
            }
            break;
            
         case 1: // contact closed state
            if (isHigh()) {
               if (_timer.isDue()) {
                  if (!bDegraded) {
                     callCb(CxContact::EContactEvent::close, getCmd());
                  }
                  _nState = 2;
                  _timer.start(getDebounce()); // set timer for debounce
               }
               // remain in state 1 while debouncing
            } else {
               // contact was close for less than the debounce time
               _nState = 0;
            }
            break;
         case 2:
            if (isLow()) {
               if (_timer.isDue()) {
                  if (!bDegraded) {
                     callCb(CxContact::EContactEvent::open, getCmd());
                  }
                  _nState = 0;
               }
            }
            break;
      }
   }
};

class CxCounter : public CxContact {
   uint32_t _nCnt;
public:
   CxCounter(int nPin = -1, const char* name = "", bool bInverted = false, bool bPullup = false, const char* cmd = "") : CxContact(nPin, name, bInverted, bPullup, cmd, [this](CxDevice* dev, uint8_t id, const char* cmd){
      if (id == CxContact::EContactEvent::close) {
         String strCmd;
         strCmd.reserve((uint32_t)(strlen(cmd) + 10)); // preserve some space for the command and additional parameters.
         strCmd = cmd;

         _nCnt++;
         strCmd.replace(F("$COUNTER"), String(_nCnt));
         ESPConsole.processCmd(strCmd.c_str());
      }
   }), _nCnt(0) {
   }
   
   const char* getTypeSz() {return "counter";}
   
   void set(uint32_t set) {_nCnt = set;}
   void reset() {set(0);}
   uint32_t getCounter() {return _nCnt;}
   
};


#endif /* CxContact_hpp */
