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
extern CxESPStackTracker g_Stack;  // init as early as possible...

class CxESPStackTracker {
   size_t _nMax = 0x4000;
   size_t _nLow = 0x4000;

   bool _bDebugPrint = false;
   uint8_t _nDebugPrintCnt = 0;
   uint8_t _nLevel = 0;
   uint8_t _nMaxLevel = 0;

  public:
   void enableDebugPrint(bool set) {
      _bDebugPrint = set;
      _nDebugPrintCnt = 1;
   }

   void DEBUGPrint(Stream &stream, uint8_t levelinc = 0, const char *label = "") {
      _nLevel += levelinc;
      _nMaxLevel = (_nLevel > _nMaxLevel) ? _nLevel : _nMaxLevel;
#ifdef DEBUG
      if (!_bDebugPrint) return;
      stream.printf("=== %s %03d ", label, _nDebugPrintCnt++);
      stream.print(F("STACK: free: "));
      stream.print(getFree());
      stream.print(F(" Low: "));
      if (getLow() < 500) stream.print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (getLow() < 150) stream.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      stream.print(getLow());
      stream.print(ESC_ATTR_RESET);
      stream.print(ESC_ATTR_RESET);
      stream.print(F(" deep: "));
      stream.print(_nLevel);
      stream.print(F(" (max: "));
      stream.print(_nMaxLevel);
      stream.print(F(")"));
      stream.println();
#endif
   }

   void print(Stream &stream) {
      stream.print(F(ESC_ATTR_BOLD " Stack: " ESC_ATTR_RESET));
      stream.print(getFree());
      stream.print(F(" bytes free"));
      stream.print(F(ESC_ATTR_BOLD " Room: " ESC_ATTR_RESET));
      stream.print(getHeapDistance());
      stream.print(F(" bytes"));
      stream.print(F(ESC_ATTR_BOLD " Lowest: " ESC_ATTR_RESET));
      if (getLow() < 500) stream.print(F(ESC_TEXT_BRIGHT_YELLOW));
      if (getLow() < 150) stream.print(F(ESC_TEXT_BRIGHT_RED ESC_ATTR_BLINK));
      stream.print(getLow());
      stream.print(ESC_ATTR_RESET);
      stream.print(F(" bytes"));
      stream.println();
   }

   void begin() {
      _nLow = _nMax;
   }

   size_t getLow() {
      return _nLow;
   }

   size_t getFree() {
      size_t free = 0;
#ifdef ARDUINO
#ifdef ESP32
      free = uxTaskGetStackHighWaterMark(NULL);
#else
      free = ESP.getFreeContStack();
      ESP.resetFreeContStack();
#endif
#endif
      _nLow = (free < _nLow) ? free : _nLow;
      return free;
   }

   size_t getHeapDistance() {
      char stack;
      char *heap = new char(1);
      delete heap;
      return (&stack - heap);
   }
};

#endif  // CxESPStackTracker_hpp
