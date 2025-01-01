//
//  CxESPTime.hpp
//  xESP
//
//  Created by ocfu on 13.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPTime_hpp
#define CxESPTime_hpp

#ifndef ARDUINO
#include "devenv.h"
#endif

#include <time.h>                    // for time() ctime()

/// Credits:
/// https://werner.rothschopf.net/microcontroller/202103_arduino_esp32_ntp_en.htm
/// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
///

class CxESPTime {
public:
   CxESPTime(Stream& stream) : _ioStream(&stream), _strNtpServer("fritz.box"), _strTz("CET-1CEST,M3.5.0,M10.5.0/3") {_init();};
   
   void printTime() {
      _update();
      
      char buf[80];
      strftime (buf, sizeof(buf), "%H:%M:%S", &_tmLocal);
      _ioStream->print(buf);
   }
   
   void printDate() {
      _update();
      
      char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y (%Z)", &_tmLocal);
      _ioStream->print(buf);
   }
   
   void printDateTime() {
      printDate();
      _ioStream->print(" ");
      printTime();
   }
   
   void printStartTime() {
      struct tm *tmstart = localtime(&_tStart);
      
      char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", tmstart);
      _ioStream->print(buf);
   }
   
   void printFileTime(time_t cr, time_t lw) {
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      char buf[80];
      strftime (buf, sizeof(buf), "%H:%M:%S", tmlw);
      _ioStream->print(buf);
   }
   
   void printFileDate(time_t cr, time_t lw) {
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      char buf[80];
      strftime (buf, sizeof(buf), "%d.%m.%Y", tmlw);
      _ioStream->print(buf);
   }
   
   void printFileDateTime(time_t cr, time_t lw) {
      _update();

      // Dec  4 08:04
      //struct tm *tmcr = localtime(&cr);
      struct tm *tmlw = localtime(&lw);
      
      
      char buf[80];
      if (_tmLocal.tm_year != tmlw->tm_year) {
         strftime (buf, sizeof(buf), "%b %e  %Y", tmlw);
      } else {
         strftime (buf, sizeof(buf), "%b %e %H:%M", tmlw);
      }
      _ioStream->print(buf);
   }
   
   void printUpTimeISO(bool sec = true) {
      uint32_t seconds = uint32_t (millis() / 1000);
      uint32_t days = seconds / 86400;
      seconds %= 86400;
      uint32_t hours = seconds / 3600;
      seconds %= 3600;
      uint32_t minutes = seconds / 60;
      seconds %= 60;
      if (sec) {
         _ioStream->printf("%dT:%02d:%02d:%02d", days, hours, minutes, seconds);
      } else {
         _ioStream->printf("%dT:%02d:%02d", days, hours, minutes);
      }
   }


private:
   String _strNtpServer;
   String _strTz;
   Stream* _ioStream;                   // Pointer to the stream object (serial or WiFiClient)
   
   time_t _tStart = 0;

   time_t _tNow;
   struct tm _tmLocal;
   
   void _update() {
      time(&_tNow);                    // read the current time
      localtime_r(&_tNow, &_tmLocal);  // make it the local time
      if (!_tStart) {
         _tStart = _tNow - (millis() / 1000);   // set the start time one time, deduct the time system is running
      }
   }

   void _init() {
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
   };
   
};


#endif /* CxESPTime_hpp */
