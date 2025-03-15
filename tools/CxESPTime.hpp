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
   char _buf[20];
public:
   CxESPTime() : _strNtpServer("pool.ntp.org"), _strTz("GMT0") {__initTime();};
   
   void printTime(Stream& stream, bool withTZ = true) {
      __updateTime();
      
      char buf[80];
      if (withTZ) {
         strftime (buf, sizeof(buf), "%H:%M:%S (%Z)", &_tmLocal);
      } else {
         strftime (buf, sizeof(buf), "%H:%M:%S", &_tmLocal);
      }
      stream.print(buf);
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
   
   void printDate(Stream& stream) {
      __updateTime();
      
      char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y", &_tmLocal);
      stream.print(buf);
   }
   
   void printDateTime(Stream& stream) {
      printDate(stream);
      stream.print(" ");
      printTime(stream);
   }
   
   void printStartTime(Stream& stream) {
      struct tm *tmstart = localtime(&_tStart);
      
      char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", tmstart);
      stream.print(buf);
   }
   
   const char* getStartTime() {
      struct tm *tmstart = localtime(&_tStart);
      
      static char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", tmstart);
      return buf;
   }
   
   void printFileTime(Stream& stream, time_t cr, time_t lw) {
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      char buf[80];
      strftime (buf, sizeof(buf), "%H:%M:%S", tmlw);
      stream.print(buf);
   }
   
   void printFileDate(Stream& stream, time_t cr, time_t lw) {
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y", tmlw);
      stream.print(buf);
   }
   
   void printFileDateTime(Stream& stream, time_t cr, time_t lw) {
      // Dec  4 08:04
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      char buf[80];
      if (_tmLocal.tm_year != tmlw->tm_year) {
         strftime (buf, sizeof(buf), "%b %e  %Y", tmlw);
      } else {
         strftime (buf, sizeof(buf), "%b %e %H:%M", tmlw);
      }
      stream.print(buf);
   }
   
   void printUpTimeISO(Stream& stream, bool sec = true) {
      stream.print(getUpTimeISO(sec));
   }
   
   time_t getUpTimeSeconds() {
      if (_tStart > 0) {
         return time(nullptr) - _tStart;
      } else {
         return millis() / 1000;
      }
   }

   
   const char* getUpTimeISO(bool sec = true) {
      static char buf[32];
      uint32_t seconds = uint32_t (millis() / 1000);
      uint32_t days = seconds / 86400;
      seconds %= 86400;
      uint32_t hours = seconds / 3600;
      seconds %= 3600;
      uint32_t minutes = seconds / 60;
      seconds %= 60;
      if (sec) {
         snprintf(buf, sizeof(buf), "%dT:%02d:%02d:%02d", days, hours, minutes, seconds);
      } else {
         snprintf(buf, sizeof(buf), "%dT:%02d:%02d", days, hours, minutes);
      }
      return buf;
   }
   
   void printTimeToBoot(Stream& stream) {
      stream.printf("%ds", (uint32_t) _nTimeToBoot / 1000);
   }

   const char* getNtpServer() {return _strNtpServer.c_str();}
   const char* getTimeZone() {return _strTz.c_str();}

   void setNtpServer(const char* sz) {_strNtpServer = sz; __initTime();}
   void setTimeZone(const char* sz) {_strTz = sz; __initTime();}
   
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
      if (!_tStart && _bValid) {
         _nTimeToBoot = (uint32_t) millis();
         _tStart = _tNow - (_nTimeToBoot / 1000);   // set the start time one time, deduct the time system is running
      }
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
   void __initTime() {
      if (_strNtpServer.length() != 0 && _strTz.length() != 0) {
#ifdef ARDUINO
#ifdef ESP32
         // ESP32 seems to be a little more complex:
         configTime(0, 0, _strNtpServer.c_str());  // 0, 0 because we will use TZ in the next line
         setenv("TZ", _strTz, 1);                  // Set environment variable with your time zone
         tzset();
#else
         // ESP8266
         configTime(_strTz.c_str(), _strNtpServer.c_str());    // --> for the ESP8266 only
#endif
#endif
      }
   };
   
};


#endif /* CxESPTime_hpp */
