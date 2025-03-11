//
//  CxESPStackTracker.hpp
//  xESP
//
//  Created by ocfu on 09.03.25.
//  Copyright © 2025 ocfu. All rights reserved.
//

#ifndef CxESPStackTracker_hpp
#define CxESPStackTracker_hpp

class CxESPStackTracker;
extern CxESPStackTracker g_Stack; // init as early as possible...

class CxESPStackTracker {
   char *_pStack = 0;
   
   size_t _nHigh = 0;
   
public:
   
   void begin() {
      char stack;
      _pStack = &stack;
   }
   
   size_t getSize() {
      char stack;
      size_t size = _pStack - &stack;
      _nHigh = (size > _nHigh) ? size : _nHigh;
      return size;
   }
   
   void update() {
      getSize();
   }
   
   size_t getHigh() {
      return _nHigh;
   }
   
   size_t getLow() {
#ifdef ARDUINO
#ifdef ESP32
      return uxTaskGetStackHighWaterMark(NULL);
#else
      return ESP.getFreeContStack();
#endif
#else
      return 0;
#endif
   }
   
   size_t getHeapDistance() {
      char stack;
      char *heap = new char(1);
      delete heap;
      return (&stack - heap);
   }
   
   void printSize() {
      Serial.print(F("STACK SIZE "));
      Serial.print(getSize());
      Serial.print(" distance to heap ");
      Serial.println(getHeapDistance());
   }
   
   void printHigh() {
      Serial.print(F("STACK HIGH "));
      Serial.println(getHigh());
   }
   
#pragma GCC push_options
#pragma GCC optimize ("O0")
   
   void test(int len) {
      char a[len];
      if (len > 0) a[0] = '\0';
      printSize();
   }
   
#pragma GCC pop_options  // Restore optimizations
};


#endif // CxESPStackTracker_hpp
