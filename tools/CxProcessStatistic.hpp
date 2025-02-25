//
//  CxProcessStatistic.hpp
//
//
//  Created by ocfu on 23.02.25.
//

#ifndef CxProcessStatistic_hpp
#define CxProcessStatistic_hpp

class CxProcessStatistic {
   uint32_t _nLastMeasurement = 0; // Zeitmarke des letzten Messzyklus
   uint32_t _nActiveTime = 0;      // Aktive Zeit für den aktuellen Zyklus
   uint32_t _nTotalTime = 1000000; // Beobachtungszeitraum pro Zyklus (1 Sekunde in Mikrosekunden)
   uint32_t _nLoops = 0;
   uint32_t _nLoopTime = 0;
   uint32_t _navgLoopTime = 0;
   
   // Variablen für Durchschnitt
   uint32_t _nTotalActiveTime = 0; // Gesamte aktive Zeit über alle Zyklen
   uint32_t _nTotalObservationTime = 0; // Gesamtzeit über alle Zyklen
   uint32_t _nStartActive = 0;
   float _fAvgLoad = 0.0;
   float _fLoad = 0.0;

public:
   CxProcessStatistic() {init();}
   
   void init() {
      _nLastMeasurement = (uint32_t) micros(); // Startzeit initialisieren
      _nStartActive = (uint32_t) micros();     // Zeit für die aktive Aufgabe messen

   }
   
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
      
      // Wenn der Beobachtungszeitraum erreicht ist
      if (now - _nLastMeasurement >= _nTotalTime) {
         if (_nLoops > 0) _navgLoopTime = (int32_t) (_nActiveTime / _nLoops);
         
         // Update der Gesamtzeit und aktiven Zeit
         _nTotalActiveTime += _nActiveTime;
         _nTotalObservationTime += _nTotalTime;
         
         // Durchschnittliche Prozesslast berechnen
         _fAvgLoad = (_nTotalActiveTime / (float)_nTotalObservationTime);
         
         // Ausgabe der aktuellen und durchschnittlichen Prozesslast
         _fLoad = (_nActiveTime / (float)_nTotalTime);
         
         // Werte zurücksetzen für den nächsten Messzyklus
         _nActiveTime = 0;
         _nLastMeasurement = now;
         _nLoops = 0;
      } else {
         _nLoops++;
      }
   }
   
   void measureCPULoad() {
      // TODO: improve CPU load measurement
      
      uint32_t now = (uint32_t) micros();
      
      _nLoopTime = now - _nStartActive;
      
      // Zeitdifferenz zur aktiven Zeit hinzufügen
      _nActiveTime += _nLoopTime;
      
      // Wenn der Beobachtungszeitraum erreicht ist
      if (now - _nLastMeasurement >= _nTotalTime) {
         if (_nLoops > 0) _navgLoopTime = (int32_t) (_nActiveTime / _nLoops);
         
         // Update der Gesamtzeit und aktiven Zeit
         _nTotalActiveTime += _nActiveTime;
         _nTotalObservationTime += _nTotalTime;
         
         // Durchschnittliche Prozesslast berechnen
         _fAvgLoad = (_nTotalActiveTime / (float)_nTotalObservationTime);
         
         // Ausgabe der aktuellen und durchschnittlichen Prozesslast
         _fLoad = (_nActiveTime / (float)_nTotalTime);
         
         // Werte zurücksetzen für den nächsten Messzyklus
         _nActiveTime = 0;
         _nLastMeasurement = now;
         _nLoops = 0;
      } else {
         _nLoops++;
      }
      
      // Zeit für die nächste Aufgabe messen
      _nStartActive = (uint32_t) micros();
   }
};

#endif /* CxProcessStatistic_hpp */

