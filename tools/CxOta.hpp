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
#endif

#ifndef ESP_CONSOLE_NOWIFI

class CxOta;
extern CxOta Ota1;

class CxOta {
public:
   typedef void (*cb_t)();
   typedef void (*cbPrgs_t)(unsigned int, unsigned int);

private:
   bool m_bInitialized = false;
   
   cb_t _cbStart = nullptr;
   cbPrgs_t _cbProgress = nullptr;
   cb_t _cbEnd = nullptr;

public:
   bool begin(const char* szHostname, const char* szPw) {
#ifdef ARDUINO
      ArduinoOTA.setHostname(szHostname);
      ArduinoOTA.setPassword(szPw);
      
      ArduinoOTA.onStart([]() {
         Ota1.start(); // tells the app about the start (via callback)
      });
      ArduinoOTA.onEnd([]() {
         Ota1.end();
         ESP.restart();
      });
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
         int8_t p = (int8_t)round(progress * 100 / total);
         static int8_t last = 0;
         if ((p % 10)==0 && p != last) {
            //info(F("OTA: Progress %u"), p);
            last = p;
         }
         Ota1.progress(progress, total);
         //Led1.blinkData();
      });
      ArduinoOTA.onError([](ota_error_t error) {
         //error(XC_F("OTA: ### Error[%u]"), error);
         //Led1.blinkError();
         //      if (error == OTA_AUTH_ERROR) {Log.error(XC_F("OTA: ### Auth Failed"));}
         //      else if (error == OTA_BEGIN_ERROR) {Log.error(XC_F("OTA: ### Begin Failed"));}
         //      else if (error == OTA_CONNECT_ERROR) {Log.error(XC_F("OTA: ### Connect Failed"));}
         //      else if (error == OTA_RECEIVE_ERROR) {Log.error(XC_F("OTA: ### Receive Failed"));}
         //      else if (error == OTA_END_ERROR) {Log.error(XC_F("OTA: ### End Failed"));}
      });
      
      ArduinoOTA.begin();
      //info(F("OTA: ready"));
      
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
};

#endif /* ESP_CONSOLE_NOWIFI */

#endif /* CxOta_hpp */
