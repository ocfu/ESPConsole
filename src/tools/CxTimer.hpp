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

#include "CxStrToken.hpp"

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

   String _strId; ///< The timer ID


protected:
   String __strCmd; ///< The command associated with the timer

   // timer callback
   std::function<void(const char* szCmd)> __cb;
   
   bool __isCron = false;
   

public:
   explicit CxTimer(uint32_t period, bool hold = false) : _nPeriod(period), _last(0), _bOnHold(hold||(period == 0)), _isDue(false), _bHoldAfterDue(false), __cb(nullptr) {if (!hold) start();}
   CxTimer() : CxTimer(0) {}
   CxTimer(uint32_t period, std::function<void(const char*)> cb, bool bHoldAfterDue = false) : _nPeriod(period), _last(0), _bOnHold(false), _isDue(false), _bHoldAfterDue(bHoldAfterDue), __cb(cb) {if (!bHoldAfterDue) start();}


   // add a virtual destructor to allow derived classes to clean up properly
   virtual ~CxTimer() {
      // clear the callback to avoid dangling pointers
      __cb = nullptr;
   }
   
   void setId(const char* set) {_strId = set;}
   const char* getId() {return _strId.c_str();}
   
   void start() {_last = (uint32_t)millis(); _bOnHold = (_nPeriod == 0 && !__isCron);}
   void stop() {_bOnHold = true;}
   void restart() {start();}
   void reset() {stop();}
   void start(uint32_t period, bool bMakeDue = false) {_nPeriod = period; start(); if (bMakeDue) makeDue();}
   void start(bool bMakeDue) {start(); if (bMakeDue) makeDue();}
   void start(uint32_t period, std::function<void(const char*)> cb, bool bHoldAfterDue = false) {__cb = cb; start(period); _bHoldAfterDue = bHoldAfterDue;}
   void start(std::function<void(const char*)> cb, bool bHoldAfterDue = false) {__cb = cb; start(); _bHoldAfterDue = bHoldAfterDue;}
   void startOnChange(uint32_t period) {if (_nPeriod != period) start(period);}
   void makeDue() {_isDue = (_nPeriod > 0);}
   
   virtual void loop() {
      if (__cb && isDue()) {
         __cb(__strCmd.c_str());
      }
   }

   void setCmd(const char* cmd) {__strCmd = cmd;}
   const char* getCmd() {return __strCmd.c_str();}
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
   
   bool isRunning() {return (!isOnHold() && (_nPeriod || __isCron));}
   bool isCron() {return __isCron;}
   virtual const char* getCron() const {return "";}

};

class CxTimer1s : public CxTimer {
public:
   explicit CxTimer1s(bool bHold = false) : CxTimer(1000, bHold) {}
};

class CxTimer10s : public CxTimer {
public:
 explicit CxTimer10s(bool bHold = false) : CxTimer(10000, bHold) {}
};

class CxTimer60s : public CxTimer {
public:
 explicit CxTimer60s(bool bHold = false) : CxTimer(60000, bHold) {}
};

// Supported cron expression features:
// - Wildcards: * (matches all values)
// - Comma-separated lists: 1,5,10
// - Ranges: 1-5 (matches 1, 2, 3, 4, 5)
// - Steps: */5, 1-10/2, 5/15 (every Nth value, optionally within a range)
// - Combinations: 1-5,10,15-20/2 (mix of lists, ranges, and steps)
// Advanced features like "last day of month" or "nth weekday" are not supported.
class CxTimerCron : public CxTimer {
   String _strCronExpr;
   bool _isValid;
   
   uint64_t _cronMinuteMask = 0; // 0-59
   uint32_t _cronHourMask = 0;   // 0-23
   uint32_t _cronDayMask = 0;    // 1-31 (bit 0 unused)
   uint32_t _cronMonthMask = 0;  // 1-12 (bit 0 unused)
   uint32_t _cronWeekdayMask = 0; // 0-6
   
   time_t _lastCronTrigger = 0;
   
   // Helper to set bits for a range with step
   template<typename T>
   void setRangeStep(T& mask, int8_t min, int8_t max, int8_t from, int8_t to, int8_t step) {
      for (int8_t i = from; i <= to; i += step) {
         if (i >= min && i <= max)
            mask |= (T(1) << i);
      }
   }
   
   // Parse a cron field with support for steps and ranges
   template<typename T>
   void setMaskBits(T& mask, int8_t min, int8_t max, const String& field) {
      if (field == "*") {
         setRangeStep(mask, min, max, min, max, 1);
         return;
      }
      CxStrToken tkField(field.c_str(), ",");
      const char* szField = tkField.get().as<const char*>();
      while (szField && *szField) {
         String part(szField);
         int8_t from = min, to = max, step = 1;
         int slashIdx = part.indexOf('/');
         if (slashIdx >= 0) {
            step = part.substring(slashIdx + 1).toInt();
            part = part.substring(0, slashIdx);
         }
         int dashIdx = part.indexOf('-');
         if (part == "*") {
            from = min; to = max;
         } else if (dashIdx >= 0) {
            from = part.substring(0, dashIdx).toInt();
            to = part.substring(dashIdx + 1).toInt();
         } else if (part.length() > 0) {
            from = to = part.toInt();
         }
         setRangeStep(mask, min, max, from, to, step);
         szField = tkField.next().as<const char*>();
      }
   }

   void parseCron(const char* expr) {
      CxStrToken tkExpr(expr, " ");
      if (tkExpr.count() != 5) return;
      _cronMinuteMask = 0;
      _cronHourMask = 0;
      _cronDayMask = 0;
      _cronMonthMask = 0;
      _cronWeekdayMask = 0;
      setMaskBits(_cronMinuteMask, 0, 59, TKTOCHAR(tkExpr, 0));
      setMaskBits(_cronHourMask, 0, 23, TKTOCHAR(tkExpr, 1));
      setMaskBits(_cronDayMask, 1, 31, TKTOCHAR(tkExpr, 2));
      setMaskBits(_cronMonthMask, 1, 12, TKTOCHAR(tkExpr, 3));
      setMaskBits(_cronWeekdayMask, 0, 6, TKTOCHAR(tkExpr, 4));
      _isValid = true;
   }
   
   bool matchCron(const struct tm& t) {
      return ((_cronMinuteMask   & (1ULL << t.tm_min))  &&
              (_cronHourMask     & (1UL  << t.tm_hour)) &&
              (_cronDayMask      & (1UL  << t.tm_mday)) &&
              (_cronMonthMask    & (1UL  << (t.tm_mon + 1))) &&
              (_cronWeekdayMask  & (1UL  << t.tm_wday)));
   }
   
public:
 explicit CxTimerCron(const char* cronExpr, std::function<void(const char*)> cb = nullptr)
     : CxTimer(0, cb), _strCronExpr(cronExpr), _isValid(false) {
    __isCron = true;
    parseCron(cronExpr);
    time_t now = time(nullptr);
    _lastCronTrigger = now / 60;  // Initialize to current minute to avoid immediate trigger
 }

   virtual void loop() override {
      time_t now = time(nullptr);
      struct tm t;
      localtime_r(&now, &t);
      if (_isValid && matchCron(t) && (_lastCronTrigger != now / 60)) {
         _lastCronTrigger = now / 60;
         if (__cb) __cb(__strCmd.c_str());
      }
   }
   
   virtual const char* getCron() const override {
      return __isCron ? _strCronExpr.c_str() : "n/a";
   }
};


#endif /* CxTimer_hpp */
