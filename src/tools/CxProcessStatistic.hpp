/**
 * @file CxProcessStatistic.hpp
 * @brief Defines the CxProcessStatistic class for measuring CPU load and loop times in an embedded system.
 *
 * This file contains the following class:
 * - CxProcessStatistic: Manages CPU load measurement, loop time calculation, and average load over time.
 *
 * @date Created by ocfu on 23.02.25.
 */

#ifndef CxProcessStatistic_hpp
#define CxProcessStatistic_hpp


/**
 * @class CxProcessStatistic
 * @brief Manages CPU load measurement, loop time calculation, and average load over time.
 * The class calculates the CPU load based on the active time of a process within a given observation period.
 * The active time is measured in microseconds and the observation period is set in microseconds.
 * The class also calculates the average load over multiple cycles.
 */
class CxProcessStatistic {
   uint32_t _nLastMeasurement = 0; // Timestamp of the last measurement cycle
   uint32_t _nActiveTime = 0;      // Active time for the current cycle
   uint32_t _nTotalTime = 1000000; // Observation period per cycle (1 second in microseconds)
   uint32_t _nLoops = 0;
   uint32_t _nLoopTime = 0;
   uint32_t _navgLoopTime = 0;
   
   // Variables for average calculation
   uint32_t _nTotalActiveTime = 0; // Total active time over all cycles
   uint32_t _nTotalObservationTime = 0; // Total time over all cycles
   uint32_t _nStartActive = 0;
   float _fAvgLoad = 0.0;
   float _fLoad = 0.0;
   
   void _init() {
      _nLastMeasurement = (uint32_t) micros(); // Initialize start time
      _nStartActive = (uint32_t) micros();     // Measure time for the active task
   }
   
public:
   CxProcessStatistic() {_init();}
   
   float load() {return _fLoad;}
   uint32_t looptime() { return _nLoopTime;}
   float avgload() {return _fAvgLoad;}
   uint32_t avglooptime() {return _navgLoopTime;}
   
   void startMeasure() {
      _nStartActive = (uint32_t) micros();
   }
   
   void stopMeasure() {
      uint32_t now = (uint32_t) micros();
      
      _nLoopTime = now - _nStartActive;
      _nActiveTime += _nLoopTime;
      
      // If the observation period is reached
      if (now - _nLastMeasurement >= _nTotalTime) {
         if (_nLoops > 0) _navgLoopTime = (int32_t) (_nActiveTime / _nLoops);
         
         // Update total time and active time
         _nTotalActiveTime += _nActiveTime;
         _nTotalObservationTime += _nTotalTime;
         
         // Calculate average process load
         _fAvgLoad = (_nTotalActiveTime / (float)_nTotalObservationTime);
         
         // Output current and average process load
         _fLoad = (_nActiveTime / (float)_nTotalTime);
         
         // Reset values for the next measurement cycle
         _nActiveTime = 0;
         _nLastMeasurement = now;
         _nLoops = 0;
      } else {
         _nLoops++;
      }
   }
   
   void measureCPULoad() {
      uint32_t now = (uint32_t) micros();
      
      _nLoopTime = now - _nStartActive;
      _nActiveTime += _nLoopTime;
      
      // If the observation period is reached
      if (now - _nLastMeasurement >= _nTotalTime) {
         if (_nLoops > 0) _navgLoopTime = (int32_t) (_nActiveTime / _nLoops);
         
         // Update total time and active time
         _nTotalActiveTime += _nActiveTime;
         _nTotalObservationTime += _nTotalTime;
         
         // Calculate average process load
         _fAvgLoad = (_nTotalActiveTime / (float)_nTotalObservationTime);
         
         // Output current and average process load
         _fLoad = (_nActiveTime / (float)_nTotalTime);
         
         // Reset values for the next measurement cycle
         _nActiveTime = 0;
         _nLastMeasurement = now;
         _nLoops = 0;
      } else {
         _nLoops++;
      }
      
      // Measure time for the next task
      _nStartActive = (uint32_t) micros();
   }
};

#endif /* CxProcessStatistic_hpp */
