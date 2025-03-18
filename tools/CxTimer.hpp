/**
 * @file CxTimer.hpp
 * @brief Timer class for ESP-based projects
 * @details This file defines the `CxTimer` class, which provides a timer capability for ESP-based projects.
 * The class includes methods for setting up, managing, and executing timers.
 *
 * @date created by ocfu on 13.12.24.
 * @copyright © 2024 ocfu
 *
 */
#ifndef CxTimer_hpp
#define CxTimer_hpp

/**
 * @class CxTimer
 * @brief Timer class for managing time intervals.
 * @details The `CxTimer` class provides a timer capability for ESP-based projects.
 * It includes methods for setting up, managing, and executing timers.
 */
class CxTimer {
   uint32_t _nPeriod; ///< The timer period in milliseconds
   uint32_t _last; ///< The time of the last timer start
   bool _bOnHold; ///< Flag to indicate if the timer is on hold
   bool _isDue;  ///< if true, trigger the next isDue() call
   
public:
   CxTimer(uint32_t period, bool hold = false) : _nPeriod(period), _last(0), _bOnHold(hold||(period == 0)), _isDue(false) {if (!hold) start();}
   CxTimer() : CxTimer(0) {}
   
   void start() {_last = (uint32_t)millis(); _bOnHold = (_nPeriod == 0);}
   void stop() {_bOnHold = true;}
   void restart() {start();}
   void start(uint32_t period, bool bMakeDue = false) {_nPeriod = period; start(); if (bMakeDue) makeDue();}
   void start(bool bMakeDue) {start(); if (bMakeDue) makeDue();}
   void startOnChange(uint32_t period) {if (_nPeriod != period) start(period);}
   void makeDue() {_isDue = (_nPeriod > 0);}

   uint32_t getElapsedTime() {return ((uint32_t)millis() - _last);}
   uint32_t getPeriod() {return _nPeriod;}
   void setPeriod(uint32_t set) {_nPeriod = set;}
   
   /** Check if the timer is due and restart it if not on hold */
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
