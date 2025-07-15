/**
 * @file CxESPTime.hpp
 * @brief Defines the CxESPTime class for managing time and date functionalities, including NTP server synchronization and time zone settings.
 *
 * This file contains the following class:
 * - CxESPTime: Provides methods to print and retrieve the current time, date, and system uptime in various formats.
 *
 * @date Created by ocfu on 13.12.24.
 */

#ifndef CxESPTime_hpp
#define CxESPTime_hpp

#ifndef ARDUINO
#include "devenv.h"
#endif

#include <time.h>                    // for time() ctime()
#include <sys/time.h>                // struct timeval
#include "CxTimer.hpp"
#include "CxTablePrinter.hpp"
#include <vector>


/// Credits:
/// https://werner.rothschopf.net/microcontroller/202103_arduino_esp32_ntp_en.htm
/// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
///

/**
 * @class CxESPTime
 * @brief Provides methods to manage time and date functionalities, including NTP server synchronization and time zone settings.
 *      The class supports printing and retrieving the current time, date, and system uptime in various formats.
 *      It also includes methods for printing file timestamps and system boot time.
 */
class CxESPTime {
   char _buf[32];
   bool _bSynced = false;
   
   std::vector<CxTimer*> _timers;
   std::function<void()> _cbSynced = nullptr;
   
public:
   CxESPTime() : _strNtpServer(F("")), _strTz(F("UTC")) {
      memset(_buf, 0, sizeof(_buf));
      memset(&_tmLocal, 0, sizeof(_tmLocal));
      _cbSynced = [this]() {
         if (!_tStart) {
            time(&_tNow);
            _nTimeToBoot = (uint32_t) millis();
            _tStart = _tNow - (_nTimeToBoot / 1000);   // set the start time one time, deduct the time system is running
         }
         _bSynced = true;
      };
      __initTime();
   };
   
   bool isSynced() {return _bSynced;}
   
   bool addTimer(CxTimer* pTimer) {
      if (pTimer) {
         // Check, if the timer has an id
         if (!pTimer->getId()[0]) {
            static uint8_t nId = 1;
            char szId[10];
            snprintf(szId, sizeof(szId), "_t%d", nId++);
            pTimer->setId(szId);
         }
         
         // Check if the timer ID already exists
         for (const auto& timer : _timers) {
            if (timer && (strcmp(timer->getId(), pTimer->getId()) == 0)) {
               return false; // Timer ID already exists
            }
         }
         _timers.push_back(pTimer);
         return true;
      }
      return false;
   }
 
   bool delTimer(const char* szId) {
      for (auto it = _timers.begin(); it != _timers.end(); ++it) {
         if (strcmp((*it)->getId(), szId) == 0) {
            delete *it;
            _timers.erase(it);
            return true;
         }
      }
      return false;
   }
   
   void startTimer(const char* szId) {
      for (auto& timer : _timers) {
         if (timer != nullptr && (strcmp(timer->getId(), szId) == 0)) {
            timer->start();
         }
      }
   }
   
   void stopTimer(const char* szId) {
      for (auto& timer : _timers) {
         if (timer != nullptr && (strcmp(timer->getId(), szId) == 0)) {
            timer->stop();
         }
      }
   }
    
   void delAllTimers() {
      for (auto& timer : _timers) {
         delete timer;
      }
      _timers.clear();
   }
   
   void loopTimers() {
      for (auto& timer : _timers) {
         if (timer != nullptr) {
            timer->loop();
         }
      }
   }
   
   CxTimer* getTimer(const char* szId) {
      for (auto& timer : _timers) {
         if (timer != nullptr && (strcmp(timer->getId(), szId) == 0)) {
            return timer;
         }
      }
      return nullptr;
   }
      
   uint32_t convertToMilliseconds(const char* szPeriod) {
      if (szPeriod == nullptr || szPeriod[0] == '\0') return 0; // Handle null or empty input
      size_t len = strlen(szPeriod);
      char unit = szPeriod[len - 1];
      uint32_t value = atoi(szPeriod);
      if (unit == 'd') return value * 86400000; // Add support for days
      if (unit == 'h') return value * 3600000;
      if (unit == 'm') return value * 60000;
      if (unit == 's') return value * 1000;
      return value; // Default to milliseconds
   }
   
   void convertToHumanReadableTime(uint32_t value, char* buf, size_t bufSize) {
      if (!buf || bufSize == 0) return; // Handle invalid buffer
      if (value >= 86400000) { // Days
         snprintf(buf, bufSize, "%.1fd", value / 86400000.0);
      } else if (value >= 3600000) { // Hours
         snprintf(buf, bufSize, "%.1fh", value / 3600000.0);
      } else if (value >= 60000) { // Minutes
         snprintf(buf, bufSize, "%.1fm", value / 60000.0);
      } else if (value >= 1000) { // Seconds
         snprintf(buf, bufSize, "%.1fs", value / 1000.0);
      } else { // Milliseconds
         snprintf(buf, bufSize, "%ums", value);
      }
   }
   
   void printTimers(Stream& stream) {
      CxTablePrinter table(stream);
      table.printHeader({F("Id"), F("Time"), F("Mode"), F("Remain"), F("Cmd")}, {10, 10, 6, 7, 60});
      char* szTime = new char[15];
      char* szRemain = new char[15];
      for (uint8_t i = 0; i < _timers.size(); i++) {
         if (_timers[i] != nullptr) {
            if (_timers[i]->isCron()) {
               snprintf(szTime, 15, "%s", _timers[i]->getCron());
            } else {
               convertToHumanReadableTime(_timers[i]->getPeriod(), szTime, 15);
            }
            if (!_timers[i]->isRunning()) {
               snprintf(szRemain, 15, "Stopped");
            } else {
               if (_timers[i]->isCron()) {
                  snprintf(szRemain, 15, "-");
               } else {
                  convertToHumanReadableTime(_timers[i]->getRemain(), szRemain, 15);
               }
            }
            table.printRow({_timers[i]->getId(), szTime, _timers[i]->getModeSz(), szRemain, _timers[i]->getCmd()});
         }
      }
      delete[] szTime;
      delete[] szRemain;
   }
      
   const char* printTime(Stream& stream, bool withTZ = true) {
      __updateTime();
      
      if (withTZ) {
         strftime (_buf, sizeof(_buf), "%H:%M:%S (%Z)", &_tmLocal);
      } else {
         strftime (_buf, sizeof(_buf), "%H:%M:%S", &_tmLocal);
      }
      stream.print(_buf);
      return _buf;
   }
   
   /**
    * @brief Retrieves the current time in the format "HH:MM:SS" or "HH:MM:SS.SSS" with milliseconds.
    * @param ms Flag to include milliseconds in the time string.
    * @return The current time as a string.
    * @note The time string is stored in a static buffer and is overwritten with each call to this method.
    * @note The time is retrieved from the system clock using the time() function.
    * @note If the time is not valid, the method returns the system uptime in the format "HH:MM:SS.SSS".
    */
   const char* getTime(bool ms = false) {
      __updateTime();
      if (isValid()) {
         strftime (_buf, sizeof(_buf), "%H:%M:%S", &_tmLocal);
         if (ms) {
            unsigned long millisec;
            struct timeval tv;
            
            gettimeofday(&tv, NULL);
            millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
            snprintf(_buf+8, sizeof(_buf)-8, ".%03lu", millisec);
         }
      } else {
         uint32_t millisec = uint32_t (millis());
         millisec %= 1000;
         uint32_t seconds = uint32_t (millis() / 1000);
         seconds %= 86400;
         uint32_t hours = seconds / 3600;
         seconds %= 3600;
         uint32_t minutes = seconds / 60;
         seconds %= 60;
         snprintf(_buf, sizeof(_buf), "%02d:%02d:%02d.%03d", hours, minutes, seconds, millisec);
      }
      return _buf;
   }
   
   const char* printDate(Stream& stream) {
      __updateTime();
      
      strftime (_buf, sizeof(_buf), "%d.%m.%Y", &_tmLocal);
      stream.print(_buf);
      return _buf;
   }
   
   void printDateTime(Stream& stream) {
      printDate(stream);
      stream.print(" ");
      printTime(stream);
   }
   
   const char* printStartTime(Stream& stream) {
      struct tm *tmstart = localtime(&_tStart);
      
      strftime (_buf, sizeof(_buf), "%d.%m.%Y %H:%M:%S", tmstart);
      stream.print(_buf);
      return _buf;
   }
   
   const char* getStartTime() {
      struct tm *tmstart = localtime(&_tStart);
      
      strftime (_buf, sizeof(_buf), "%Y-%m-%dT%H:%M:%S%z", tmstart);
      return _buf;
   }
   
   const char* printFileTime(Stream& stream, time_t cr, time_t lw) {
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
            strftime (_buf, sizeof(_buf), "%H:%M:%S", tmlw);
      stream.print(_buf);
      return _buf;
   }
   
   const char* printFileDate(Stream& stream, time_t cr, time_t lw) {
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      strftime (_buf, sizeof(_buf), "%d.%m.%Y", tmlw);
      stream.print(_buf);
      return _buf;
   }
   
   const char* printFileDateTime(Stream& stream, time_t cr, time_t lw) {
      // Dec  4 08:04
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
            if (_tmLocal.tm_year != tmlw->tm_year) {
         strftime (_buf, sizeof(_buf), "%b %e  %Y", tmlw);
      } else {
         strftime (_buf, sizeof(_buf), "%b %e %H:%M", tmlw);
      }
      stream.print(_buf);
      return _buf;
   }
   
   const char* printUpTimeISO(Stream& stream, bool sec = true) {
      stream.print(getUpTimeISO(sec));
      return _buf;
   }
   
   time_t getUpTimeSeconds() {
      if (_tStart > 0) {
         return time(nullptr) - _tStart;
      } else {
         return millis() / 1000;
      }
   }

   
   const char* getUpTimeISO(bool sec = true) {
      uint32_t seconds = uint32_t (millis() / 1000);
      uint32_t days = seconds / 86400;
      seconds %= 86400;
      uint32_t hours = seconds / 3600;
      seconds %= 3600;
      uint32_t minutes = seconds / 60;
      seconds %= 60;
      if (sec) {
         snprintf(_buf, sizeof(_buf), "%dT:%02d:%02d:%02d", days, hours, minutes, seconds);
      } else {
         snprintf(_buf, sizeof(_buf), "%dT:%02d:%02d", days, hours, minutes);
      }
      return _buf;
   }
   
   void printTimeToBoot(Stream& stream) {
      stream.printf("%ds", (uint32_t) _nTimeToBoot / 1000);
   }

   const char* getNtpServer() {return _strNtpServer.c_str();}
   const char* getTimeZone() {return _strTz.c_str();}

   bool setNtpServer(const char* sz) {
      if (sz) {
         _strNtpServer = sz;
         return __initTime();
      }
      return false;
   }
   bool setTimeZone(const char* sz) {_strTz = sz?sz:""; return __initTime();}
   
   bool isValid() {return _bValid;}
   
   int getTimeHour() {
      __updateTime();
      return _tmLocal.tm_hour;
   }
   
   int getTimeMin() {
      __updateTime();
      return _tmLocal.tm_min;
   }
   
   int getTimeSec() {
      __updateTime();
      return _tmLocal.tm_sec;
   }

protected:
   /**
    * @brief Updates the current time and date in the local time zone.
    * @details Reads the current time from the system clock and converts it to the local time zone.
    * @note The method sets the _bValid flag to true if the time is successfully synchronized.
    * @note The method sets the _tStart time to the system start time if it has not been set yet.
   */
   void __updateTime() {
      time(&_tNow);                    // read the current time
      _bValid = localtime_r(&_tNow, &_tmLocal);  // make it the local time
   }
   

private:
   String _strNtpServer;
   String _strTz;
   
   time_t _tStart = 0;
   uint32_t _nTimeToBoot = 0;

   time_t _tNow;
   struct tm _tmLocal;
   bool _bValid = false; // true, if synchronised
   
   /**
    * @brief Initializes the time and date settings.
    * @details Configures the NTP server and time zone settings.
    * @note The method is called by the constructor to initialize the time and date settings.
   */
   bool __initTime() {
      if (_strNtpServer.length() != 0) {
         if (!_strTz.length()) _strTz = "UTC";
#ifdef ARDUINO
#ifdef ESP32
         // ESP32 seems to be a little more complex:
         configTime(0, 0, _strNtpServer.c_str());  // 0, 0 because we will use TZ in the next line
         setenv("TZ", _strTz, 1);                  // Set environment variable with your time zone
         tzset();
#else
         // ESP8266
         configTime(_strTz.c_str(), _strNtpServer.c_str());    // --> for the ESP8266 only
         if (_cbSynced) settimeofday_cb(_cbSynced);
#endif
#endif
         return true;
      }
      return false;
   }
   
};


#endif /* CxESPTime_hpp */
