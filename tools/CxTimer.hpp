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
   bool _bHoldAfterDue;
   String _strCmd; ///< The command associated with the timer

   uint8_t _nId = 0; ///< The timer ID
   ///<
   // timer callback
   std::function<void(const char* szCmd)> _cb;

public:
   CxTimer(uint32_t period, bool hold = false) : _nPeriod(period), _last(0), _bOnHold(hold||(period == 0)), _isDue(false), _bHoldAfterDue(false), _cb(nullptr) {if (!hold) start();}
   CxTimer() : CxTimer(0) {}
   CxTimer(uint32_t period, std::function<void(const char*)> cb, bool bHoldAfterDue = false) : _nPeriod(period), _last(0), _bOnHold(false), _isDue(false), _bHoldAfterDue(bHoldAfterDue), _cb(cb) {if (!bHoldAfterDue) start();}

   void setId(uint8_t id) {_nId = id;}
   uint8_t getId() {return _nId;}
   
   void start() {_last = (uint32_t)millis(); _bOnHold = (_nPeriod == 0);}
   void stop() {_bOnHold = true;}
   void restart() {start();}
   void reset() {stop();}
   void start(uint32_t period, bool bMakeDue = false) {_nPeriod = period; start(); if (bMakeDue) makeDue();}
   void start(bool bMakeDue) {start(); if (bMakeDue) makeDue();}
   void start(uint32_t period, std::function<void(const char*)> cb, bool bHoldAfterDue = false) {_cb = cb; start(period); _bHoldAfterDue = bHoldAfterDue;}
   void start(std::function<void(const char*)> cb, bool bHoldAfterDue = false) {_cb = cb; start(); _bHoldAfterDue = bHoldAfterDue;}
   void startOnChange(uint32_t period) {if (_nPeriod != period) start(period);}
   void makeDue() {_isDue = (_nPeriod > 0);}
   
   void loop() {
      if (_cb && isDue()) {
         _cb(_strCmd.c_str());
      }
   }

   void setCmd(const char* cmd) {_strCmd = cmd;}
   const char* getCmd() {return _strCmd.c_str();}
   const char* getModeSz() {return _bHoldAfterDue ? "once" : "repeat";}
   uint8_t getMode() {return _bHoldAfterDue ? 0 : 1;}
   uint32_t getRemain() {return _nPeriod - ((uint32_t)millis() - _last);}
   
   uint32_t getElapsedTime() {return ((uint32_t)millis() - _last);}
   uint32_t getPeriod() {return _nPeriod;}
   void setPeriod(uint32_t set) {_nPeriod = set;}
   
   /** Check if the timer is due and restart it if not on hold */
   bool isDue(bool hold = false) {
      if (!_bOnHold && (_isDue || (millis() - _last) > _nPeriod)) {
         if (_bHoldAfterDue) hold = true;
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
