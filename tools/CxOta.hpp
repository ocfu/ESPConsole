//
//  CxOta.hpp
//  test
//
//  Created by ocfu on 31.07.22.
//

#ifndef CxOta_hpp
#define CxOta_hpp

#ifdef ARDUINO
#include <ArduinoOTA.h>
#else
#define ota_error_t int
#endif

#ifndef ESP_CONSOLE_NOWIFI

class CxOta;
extern CxOta Ota1;

class CxOta {
public:
   typedef void (*cb_t)();
   typedef void (*cbPrgs_t)(unsigned int, unsigned int);
   typedef void (*cbErr_t)(ota_error_t error);
   
private:
   bool m_bInitialized = false;
   
   cb_t _cbStart = nullptr;
   cbPrgs_t _cbProgress = nullptr;
   cb_t _cbEnd = nullptr;
   cbErr_t _cbError = nullptr;

public:
   bool begin(const char* szHostname, const char* szPw) {
#ifdef ARDUINO
      ArduinoOTA.setHostname(szHostname);
      ArduinoOTA.setPassword(szPw);
      
      ArduinoOTA.onStart([]() {
         Ota1.start(); // inform the console through cb
      });
      ArduinoOTA.onEnd([]() {
         Ota1.end(); // inform the console through cb
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
         int8_t p = (int8_t)round(progress * 100 / total);
         static int8_t last = 0;
         if ((p % 10)==0 && p != last) {
            // only report 10% steps to the console cb
            Ota1.progress(progress, total);
            last = p;
         }
      });
      ArduinoOTA.onError([](ota_error_t error) {
         Ota1.error(error);
      });
      
      ArduinoOTA.begin();
      
      m_bInitialized = true;
#endif
      return true;
   }

   void loop() {
#ifdef ARDUINO
      if (m_bInitialized && WiFi.status() == WL_CONNECTED) {
         ArduinoOTA.handle();
      }
#endif
   }
   
   void onStart(cb_t cb){_cbStart = cb;}
   void start() {if(_cbStart) _cbStart();}
   void onProgress(cbPrgs_t cb){_cbProgress = cb;}
   void progress(unsigned int p, unsigned int t) {if(_cbProgress) _cbProgress(p, t);}
   void onEnd(cb_t cb){_cbEnd = cb;}
   void end() {if(_cbEnd) _cbEnd();}
   void onError(cbErr_t cb){_cbError = cb;}
   void error(ota_error_t error) {if(_cbError) _cbError(error);}
};

#endif /* ESP_CONSOLE_NOWIFI */

#endif /* CxOta_hpp */
