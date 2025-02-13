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
   size_t _nInitialHeap;
public:
   CxESPHeapTracker() {_nInitialHeap = available();};
   size_t size() {
      return _nInitialHeap;
   }
   size_t available() {
#ifdef ARDUINO
      return ESP.getFreeHeap();
#else
      return 0;
#endif
   }
   size_t used() {
      return _nInitialHeap - available();
   }
   
   size_t fragmentation() {
#ifdef ARDUINO
      return ESP.getHeapFragmentation();
#else
      return 0;
#endif
   }
   
};

#endif /* CxESPHeapTracker_hpp */
