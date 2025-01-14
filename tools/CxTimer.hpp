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
   bool _bFinish;  // if true, trigger the next isDue() call
   
public:
   CxTimer(uint32_t period, bool hold = false) : _nPeriod(period), _last(0), _bOnHold(hold||(period == 0)), _bFinish(false) {if (!hold) start();}
   CxTimer() : CxTimer(0) {}
   
   void start() {_last = (uint32_t)millis(); _bOnHold = (_nPeriod == 0);}
   void restart() {start();}
   void start(uint32_t period) {_nPeriod = period; start();}
   void startOnChange(uint32_t period) {if (_nPeriod != period) start(period);}
   void finish() {_bFinish = true;}

   uint32_t getElapsedTime() {return ((uint32_t)millis() - _last);}
   uint32_t getPeriod() {return _nPeriod;}
   
   bool isDue(bool hold = false) {
      if (!_bOnHold && (_bFinish || (millis() - _last) > _nPeriod)) {
         if (!hold) restart();
         _bOnHold = hold;
         _bFinish = false;
         return true;
      }
      return false;
   }
   
   bool isOnHold() {return _bOnHold;}
   
   bool isRunning() {return (!isOnHold() && _nPeriod);}
   
};

class CxTimer1s : public CxTimer {
public:
   CxTimer1s() : CxTimer(1000) {}
};

class CxTimer10s : public CxTimer {
public:
   CxTimer10s() : CxTimer(10000) {}
};

class CxTimer60s : public CxTimer {
public:
   CxTimer60s() : CxTimer(60000) {}
};

#endif /* CxTimer_hpp */
