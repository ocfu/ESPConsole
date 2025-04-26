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
   
   bool _bDebugPrint = false;
   uint8_t _nDebugPrintCnt = 0;
   
public:
   
   void enableDebugPrint(bool set) {_bDebugPrint = set; _nDebugPrintCnt = 1;}
   
   void DEBUGPrint(Stream &stream, const char* label = "") {
#ifdef DEBUG_BUILD
      if (!_bDebugPrint) return;
      stream.printf("=== %s %03d ", label, _nDebugPrintCnt++);
      stream.print(F("STACK: "));
      stream.print(getSize());
      stream.print(F(" LWM: "));
      if (getLow() < 500) stream.print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (getLow() < 150) stream.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      stream.print(getLow());
      stream.print(ESC_ATTR_RESET);
      stream.print(F(" MAX: "));
      if (getHigh() > 1500) stream.print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (getHigh() > 2500) stream.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      stream.println(getHigh());
      stream.print(ESC_ATTR_RESET);
#endif
   }
   
   void print(Stream &stream) {
      stream.print(F(ESC_ATTR_BOLD " Stack: " ESC_ATTR_RESET));stream.print(getSize());stream.print(F(" bytes"));
      stream.print(F(ESC_ATTR_BOLD " Room: " ESC_ATTR_RESET));stream.print(getHeapDistance());stream.print(F(" bytes"));
      stream.print(F(ESC_ATTR_BOLD " High: " ESC_ATTR_RESET));
      if (getHigh() > 1500) stream.print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (getHigh() > 2500) stream.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      stream.print(getHigh());
      stream.print(ESC_ATTR_RESET);
      stream.print(F(" bytes"));
      stream.print(F(ESC_ATTR_BOLD " Low: " ESC_ATTR_RESET));
      if (getLow() < 500) stream.print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (getLow() < 150) stream.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      stream.print(getLow());
      stream.print(ESC_ATTR_RESET);
      stream.print(F(" bytes"));
      stream.println();
   }
   
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
   
#ifdef DEBUG_BUILD
#pragma GCC push_options
#pragma GCC optimize ("O0")
   
   void test(int len) {
      char a[len];
      if (len > 0) a[0] = '\0';
      printSize();
   }
   
#pragma GCC pop_options  // Restore optimizations
#endif
};


#endif // CxESPStackTracker_hpp
