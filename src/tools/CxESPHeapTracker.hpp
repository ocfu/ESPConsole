//
//  CxESPHeapTracker.hpp
//  xESP
//
//  Created by ocfu on 13.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxESPHeapTracker_hpp
#define CxESPHeapTracker_hpp

class CxESPHeapTracker;
extern CxESPHeapTracker g_Heap; // init as early as possible...

class CxESPHeapTracker {
   size_t _nInitialHeap = 0;
   size_t _nActualHeap = 0;
   size_t _nActualFrag = 0;
   size_t _nLowHeap = 0;
   size_t _nFragPeak = 0;
   
public:
   CxESPHeapTracker(size_t init = 0) {
      if (init) {
         _nInitialHeap = init;
      } else {
         _nInitialHeap = update();
      }
      _nLowHeap = _nInitialHeap;
   };
   
   size_t size() {
      return _nInitialHeap;
   }
   
   size_t available(bool bForceUpdate = false) {
      // Update shall be called generally only by one instance in a loop.
      // why? ESP.getFreeHeap() seems to depend on a context and this leads
      // to different results.
      if (bForceUpdate) {
         update();
      }
      return _nActualHeap;
   }
   
   size_t used() {
      return size() - available();
   }
   
   size_t fragmentation() {
      return _nActualFrag;
   }
   
   size_t update() {
#ifdef ARDUINO
      _nActualHeap = ESP.getFreeHeap();
      _nActualFrag = ESP.getHeapFragmentation();
#endif
      if (_nActualHeap < _nLowHeap) _nLowHeap = _nActualHeap;
      if (_nActualFrag > _nFragPeak) _nFragPeak = _nActualFrag;

      return _nActualHeap;
   }
   
   size_t low() {
      return _nLowHeap;
   }
   
   size_t peak() {
      return _nFragPeak;
   }
   
};

#endif /* CxESPHeapTracker_hpp */
