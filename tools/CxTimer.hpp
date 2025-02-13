//
//  CxTimer.hpp
//  xESP
//
//  Created by ocfu on 25.09.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxTimer_hpp
#define CxTimer_hpp

class CxTimer {
   uint32_t _nPeriod;
   uint32_t _last;
   bool _bOnHold;
   bool _isDue;  // if true, trigger the next isDue() call
   
public:
   CxTimer(uint32_t period, bool hold = false) : _nPeriod(period), _last(0), _bOnHold(hold||(period == 0)), _isDue(false) {if (!hold) start();}
   CxTimer() : CxTimer(0) {}
   
   void start() {_last = (uint32_t)millis(); _bOnHold = (_nPeriod == 0);}
   void stop() {_bOnHold = true;}
   void restart() {start();}
   void start(uint32_t period, bool bMakeDue = false) {_nPeriod = period; start(); if (bMakeDue) makeDue();}
   void start(bool bMakeDue) {start(); if (bMakeDue) makeDue();}
   void startOnChange(uint32_t period) {if (_nPeriod != period) start(period);}
   void makeDue() {_isDue = true;}

   uint32_t getElapsedTime() {return ((uint32_t)millis() - _last);}
   uint32_t getPeriod() {return _nPeriod;}
   void setPeriod(uint32_t set) {_nPeriod = set;}
   
   bool isDue(bool hold = false) {
      if (!_bOnHold && (_isDue || (millis() - _last) > _nPeriod)) {
         if (!hold) restart();
         _bOnHold = hold;
         _isDue = false;
         return true;
      }
      return false;
   }
   
   bool isOnHold() {return _bOnHold;}
   
   bool isRunning() {return (!isOnHold() && _nPeriod);}
   
};

class CxTimer1s : public CxTimer {
public:
   CxTimer1s(bool bHold = false) : CxTimer(1000, bHold) {}
};

class CxTimer10s : public CxTimer {
public:
   CxTimer10s(bool bHold = false) : CxTimer(10000, bHold) {}
};

class CxTimer60s : public CxTimer {
public:
   CxTimer60s(bool bHold = false) : CxTimer(60000, bHold) {}
};

#endif /* CxTimer_hpp */
