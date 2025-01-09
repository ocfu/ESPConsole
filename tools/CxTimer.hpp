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
   uint32_t _interval;
   uint32_t _last;
   bool _bOnHold;
   bool _bFinish;  // if true, trigger the next isDue() call
   
public:
   CxTimer(uint32_t interval) : _interval(interval), _last(0), _bOnHold((interval == 0)), _bFinish(false) {start();}
   CxTimer() : CxTimer(0) {}
   
   void start() {_last = (uint32_t)millis(); _bOnHold = false;}
   void restart() {start();}
   void start(uint32_t interval) {_interval = interval; start();}
   void finish() {_bFinish = true;}
   
   bool isDue(bool hold = false) {
      if (!_bOnHold && (_bFinish || (millis() - _last) > _interval)) {
         start();
         _bOnHold = hold;
         _bFinish = false;
         return true;
      }
      return false;
   }
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
