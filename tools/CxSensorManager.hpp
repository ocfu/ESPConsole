/**
 * @file CxSensorManager.hpp
 * @brief Defines classes for managing sensors and their capabilities in an ESP system.
 * @details
 * This file contains the following classes:
 * - CxSensor: Represents a generic sensor with properties and methods for sensor management.
 * - CxSensorGeneric: Represents a generic sensor with a callback function for reading values.
 * - CxSensorManager: Manages sensor capabilities, including initialization, updating, and command execution.
 *
 * The code includes conditional compilation for Arduino-specific libraries and functions.
 *
 * @date Created by ocfu on 03.03.25.
 * @copyright © 2025 ocfu. All rights reserved.
 *
 * Key Features:
 * 1. Classes and Enumerations:
 *    - ECSensorType: Enumeration of sensor types (none, temperature, humidity, pressure, flow).
 *    - CxSensor: Base class for all sensor types, providing common properties and methods for sensor management.
 *    - CxSensorGeneric: Derived class of CxSensor with a callback function for reading values.
 *    - CxSensorManager: Singleton class that manages sensors, including initialization, updating, and command execution.
 *
 * 2. CxSensor Class:
 *    - Manages sensor properties such as type, resolution, name, model, and value.
 *    - Provides methods to enable/disable sensors, set/get sensor values, and update sensor readings.
 *    - Registers and unregisters sensors with the CxSensorManager.
 *
 * 3. CxSensorGeneric Class:
 *    - Extends CxSensor with a callback function to read sensor values.
 *    - Implements the begin and read methods.
 *
 * 4. CxSensorManager Class:
 *    - Manages a collection of sensors using a map with unique IDs.
 *    - Provides methods to add, remove, and retrieve sensors.
 *    - Updates sensor values and prints a list of all sensors to the console.
 *
 * Relationships:
 * - CxSensor is the base class for all sensor types and interacts with CxSensorManager to register/unregister sensors.
 * - CxSensorGeneric inherits from CxSensor and provides a specific implementation for reading sensor values using a callback function.
 * - CxSensorManager is a singleton class that manages all sensors, providing methods to add, remove, and update sensors.
 *
 * How They Work Together:
 * - CxSensor and its derived classes (e.g., CxSensorGeneric) represent individual sensors with specific properties and methods.
 * - CxSensorManager maintains a collection of these sensors, allowing for centralized management and updates.
 * - Sensors register themselves with the CxSensorManager upon creation and unregister upon destruction.
 * - The CxSensorManager can update all sensors or specific sensors, retrieve sensor values, and print sensor information to the console.
 *
 * Suggested Improvements:
 * 1. Error Handling:
 *    - Add error handling for edge cases, such as invalid sensor IDs or failed sensor updates.
 *
 * 2. Code Refactoring:
 *    - Improve code readability and maintainability by refactoring complex methods and reducing code duplication.
 *
 * 3. Documentation:
 *    - Enhance documentation with more detailed explanations of methods and their parameters.
 *
 * 4. Testing:
 *    - Implement unit tests to ensure the reliability and correctness of the sensor management functionality.
 *
 * 5. Resource Management:
 *    - Monitor and optimize resource usage, such as memory and processing time, especially for embedded systems with limited resources.
 *
 * 6. Extensibility:
 *    - Provide a more flexible mechanism for adding new sensor types and capabilities without modifying the core classes.
 */
#ifndef CxSensorManager_hpp
#define CxSensorManager_hpp

#include "CxESPConsole.hpp"
#include "CxConfigParser.hpp"

#include <map>

/**
 * @brief Enumeration of sensor types.
 */
enum class ECSensorType {
   none = 0,
   temperature,
   humidity,
   pressure,
   flow
};

/**
 * @class CxSensor
 * @brief Represents a generic sensor with properties and methods for sensor management.
 * This class is the base class for all sensor types.
 * It provides common properties and methods for sensor management.
 * Derived classes should implement specific sensor types and methods.
 * The class also registers and unregisters sensors with the sensor manager. *
 */
class CxSensor {
   
   bool _bEnabled = true;  /// Indicates if the sensor is enabled
   
   ECSensorType _eType = ECSensorType::none;  /// Type of the sensor

protected:
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();  /// Reference to the console instance
   
   unsigned long __nLastUpdate = 0;  /// Timestamp of the last update
   uint8_t __nResolution = 12;  /// Resolution of the sensor (9-12 bits)
   unsigned long __nTimeToConvert = 100;  /// Time required for the sensor to convert/read the measurement
   
   String __strName;  /// Name of the sensor
   String __strUnit;  /// Unit of the sensor measurement
   String __strModel;  /// Model of the sensor
   uint64_t __nId = 0;  /// Unique ID of the sensor
   
   float __fMaxValue = 9999.999;  /// Maximum value the sensor can read
   float __fMinValue = -9999.999;  /// Minimum value the sensor can read
   float __fResolution = 0.0;  /// Resolution of the sensor value
   
   float __fValue = INVALID_FLOAT;  /// Current value of the sensor
   int32_t __nValue = INVALID_INT32;  /// Current value of the sensor as an integer
   bool __bValid = false;  /// Indicates if the sensor is valid
   bool __bValidValue = false;  /// Indicates if the sensor value is valid
   
   /// Register the sensor with the manager
   void registerSensors();
   
   /// Unregister the sensor from the manager
   void unregisterSensors();
   
public:
   CxSensor() {}
   CxSensor(ECSensorType eType, uint8_t res = 12) {begin(eType, res);}
   virtual ~CxSensor() {end();}
   
   /// Pure virtual method to initialize the sensor
   virtual bool begin() = 0;
   bool begin(ECSensorType eType, uint8_t res = 12) {setType(eType); setResolution(res); return begin();}
   void end() {
      __bValid = false;
      __bValidValue = false;
   }
   
   /// Set the enabled state of the sensor
   void setEnabled(bool set = true) {_bEnabled = set;}
   /// Check if the sensor is enabled
   bool isEnabled() {return _bEnabled;}
   
   /// Set the unique ID of the sensor
   void setId(int set) {__nId = set;}
   /// Get the unique ID of the sensor
   uint64_t getId() {return __nId;}
   
   /// Get the type of the sensor
   ECSensorType getType() {return _eType;}
   /// Set the type of the sensor
   void setType(ECSensorType eType) {_eType = eType;}
   
   /// Get the type of the sensor as a string
   const char* getTypeSz() {
      switch (_eType) {
         case ECSensorType::temperature: return "temperature";
         case ECSensorType::humidity: return "humidity";
         case ECSensorType::pressure: return "pressure";
         case ECSensorType::flow: return "flow";
         default: return "unknown";
      }
   }
   
   /// set the model of the sensor
   void setModel(const char* model) {__strModel = model;}
   
   /// Get the model of the sensor
   const char* getModel() {return __strModel.c_str();}
   
   /// Set the resolution of the sensor
   void setResolution(uint8_t set) {__nResolution = set;}
   /// Get the resolution of the sensor
   uint8_t getResolution() {return __nResolution;}
   
   /// Invalidate the sensor value
   void setInvalid() {__bValidValue = false;}
   /// Check if the sensor is valid
   bool isValid() {return (__bValid && isEnabled());}
   /// Check if the sensor has a valid value
   bool hasValidValue() {return (__bValidValue && isValid());}
   /// Get the current value of the sensor as a float
   float getFloatValue() {return __fValue;}
   /// Get the current value of the sensor as an integer
   int32_t getIntValue() {return __nValue;}
   /// Set the current value of the sensor as an integer
   void setValue(int nValue) {__nValue = nValue;}
   /// Set the current value of the sensor as a float
   void setValue(float fValue) {__fValue = fValue; __nValue = static_cast<int32_t>(round(__fValue));}
   
   /// Get the maximum value the sensor can read
   float getMaxValue() {return __fMaxValue;}
   /// Get the minimum value the sensor can read
   float getMinValue() {return __fMinValue;}
   /// Get the unit of the sensor measurement
   const char* getUnit() {return __strUnit.c_str();}
   /// Get the name of the sensor
   const char* getName() {return __strName.c_str();}
   void setName(const char* name) {__strName = name;}

   
   /// Pure virtual method to read sensor values
   virtual bool read() = 0;
   /// Update the sensor value
   bool update(){
      
      unsigned long now = millis();
      
      if ((now - __nLastUpdate) < __nTimeToConvert) {
         return false;  /// Update cycle was too quick
      }
      
      __fValue = INVALID_FLOAT;
      
      __nLastUpdate = now;
      
      if (isValid()) {
         __bValidValue = read();
         if (!__bValidValue) {
            __console.debug_ext(DEBUG_FLAG_SENSOR, F("SENS: %s (%d) value is not ok"), getName(), getId());
         }
      } else {
         return false;
      }
      
      __nValue = static_cast<uint32_t>(round(__fValue));
      
      return true;
   }
   /// Update the sensor value with a given value
   bool update(float value) {
      if (value >= __fMinValue && value <= __fMaxValue) {
         setValue(value);
         __bValidValue = true;
         return true;
      }
      return false;
   }
};

/**
 * @class CxSensorGeneric
 * @brief Represents a generic sensor with a callback function for reading values.
 * This class is a derived class of CxSensor and provides a generic sensor implementation.
 * It uses a callback function to read sensor values.
 */
class CxSensorGeneric : public CxSensor {
   typedef float (*cb_sensor_t)();
   cb_sensor_t _cb;
   
public:
   CxSensorGeneric(const char* szName, ECSensorType eType, uint8_t res = 12, const char* unit = "", cb_sensor_t cb = nullptr) : _cb(cb) {
      setType(eType);
      setResolution(res);
      __strUnit = unit; //"\x09\x43"  for °C;
      __bValid = true;
      __fMaxValue = 9999.999;
      __fMinValue = -9999.999;
      __nId = 0;
      __strName = szName;
      __strModel = "generic";

      registerSensors(); /// Register the sensor with the manager

   }
   
   /// Define virtual methods from base class.
   bool begin() {return true;}
   
   bool read() {
      if (_cb) {
         float fValue = _cb();
         
         if (fValue >= __fMinValue && fValue <= __fMaxValue) {
            __fValue = fValue;
            __nValue = static_cast<int32_t>(round(fValue));
            return true;
         }
         return false;
      }
      return hasValidValue();
   }
};

/**
 * @class CxSensorManager
 * @brief Manages sensor capabilities, including initialization, updating, and command execution.
 * This class is a singleton that manages sensors and their capabilities.
 * It provides methods to add, remove, and update sensors, as well as get sensor values.
 * The class uses a map to store sensors with unique IDs.
 */
class CxSensorManager {
private:
   /// Reference to the console instance
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();
   
   /// Map to store sensors with their unique IDs
   std::map<uint8_t, CxSensor*> _mapSensors;
   
   /// Private constructor to enforce singleton pattern
   CxSensorManager() = default;
   /// Default destructor
   ~CxSensorManager() = default;
   
public:
   /**
    * @brief Access the singleton instance of CxSensorManager.
    * @return Reference to the singleton instance.
    */
   static CxSensorManager& getInstance() {
      static CxSensorManager instance; // Constructed on first access
      return instance;
   }
   
   /// Disable copying and assignment to enforce singleton pattern
   CxSensorManager(const CxSensorManager&) = delete;
   CxSensorManager& operator=(const CxSensorManager&) = delete;
   
   /**
    * @brief Get the number of sensors in the manager.
    * @return The number of sensors.
    */
   uint8_t getSensorCount() {return _mapSensors.size();}
   
   /**
    * @brief Create a unique ID for a sensor.
    * @return A unique ID.
    */
   uint8_t createId() {
      uint8_t nId = 0;
      /// Increment ID until a unique one is found
      while (_mapSensors.find(nId) != _mapSensors.end()) {
         nId++;
      }
      return nId;
   }
   
   /**
    * @brief Add a sensor to the manager.
    * @param pSensor A unique pointer to the sensor to be added.
    */
   void addSensor(CxSensor* pSensor) {

      if (pSensor) {
         /// Create a unique ID for the sensor
         uint8_t nId = createId();
         
         /// load .sensors file
         String strValue;
         String strEnv = ".sensors";

         /// Load the sensor configuration
         if (__console.loadEnv(strEnv, strValue)) {
            CxConfigParser Config(strValue.c_str());
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, Config.getSz("json"));
            if (!error) {
               JsonArray sensors = doc["sensors"].as<JsonArray>();
               for (JsonObject sensor : sensors) {
                  uint8_t nIdSensor = sensor["id"].as<uint8_t>();
                  if (nId == nIdSensor) {
                     pSensor->setName(sensor["na"].as<const char*>());
                  }
               }
            }
         }

         /// Set the unique ID for the sensor
         pSensor->setId(nId);
         
         /// Add the sensor to the map
         _mapSensors[nId] = pSensor;
      }
   }
   
   /**
    * @brief Remove a sensor from the manager.
    * @param nId The unique ID of the sensor to remove.
    */
   void removeSensor(uint8_t nId) {
      auto it = _mapSensors.find(nId);
      if (it != _mapSensors.end()) {
         _mapSensors.erase(it);
      }
   }

   /**
    * @brief Remove a sensor from the manager.
    * @param pSensor A pointer to the sensor to remove.
    */
   void removeSensor(CxSensor* pSensor) {
      for (auto it = _mapSensors.begin(); it != _mapSensors.end(); ++it) {
         if (it->second == pSensor) {
            _mapSensors.erase(it);
            break;
         }
      }
   }
   
   /**
    * @brief Get a sensor by its unique ID.
    * @param nId The unique ID of the sensor to retrieve.
    * @return A pointer to the sensor, or nullptr if not found.
    */
   CxSensor* getSensor(uint8_t nId) {
      if (nId != INVALID_UINT8) {
         auto it = _mapSensors.find(nId);
         if (it != _mapSensors.end()) {
            return it->second;
         }
      }
      return nullptr;
   }
   
   /**
    * @brief Get a sensor by its name.
    * @param szName The name of the sensor to retrieve.
    * @return A pointer to the sensor, or nullptr if not found.
    */
   CxSensor* getSensor(const char* szName) {
      if (szName != nullptr) {
         for (const auto& [nId, pSensor] : _mapSensors) {
            if (strncmp(pSensor->getName(), szName, strlen(szName)) == 0) {
               return pSensor;
            }
         }
      }
      return nullptr;
   }
   
   
   /**
    * @brief Get the value of a sensor as an integer.
    * @param nId The unique ID of the sensor.
    * @return The sensor value as an integer.
    */
   uint32_t getSensorValueInt(uint8_t nId) {
      CxSensor* pSensor = getSensor(nId);
      if (pSensor) {
         return pSensor->getIntValue();
      }
      return INVALID_UINT32;
   }
   
   /**
    * @brief Get the value of a sensor as a float.
    * @param nId The unique ID of the sensor.
    * @return The sensor value as a float.
    */
   float getSensorValueFloat(uint8_t nId) {
      CxSensor* pSensor = getSensor(nId);
      if (pSensor) {
         return pSensor->getFloatValue();
      }
      return INVALID_FLOAT;
   }
   
   /**
    * @brief Update all sensors.
    */
   void update() {
      for (auto& [nId, pSensor] : _mapSensors) {
         pSensor->update();
      }
   }
   
   /**
    * @brief Update a specific sensor.
    * @param nId The unique ID of the sensor to update.
    */
   void update(uint8_t nId) {
      CxSensor* pSensor = getSensor(nId);
      if (pSensor) {
         pSensor->update();
      }
   }
   
   /**
    * @brief Update a specific sensor with a given value.
    * @param nId The unique ID of the sensor to update.
    * @param value The value to update the sensor with.
    */
   void update(uint8_t nId, float value) {
      CxSensor* pSensor = getSensor(nId);
      if (pSensor) {
         pSensor->update(value);
      }
   }
   
   /**
    * @brief Update a specific sensor with a given value.
    * @param nId The unique ID of the sensor to update.
    * @param value The value to update the sensor with.
    */
   void update(uint8_t nId, int32_t value) {
      CxSensor* pSensor = getSensor(nId);
      if (pSensor) {
         pSensor->update(value);
      }
   }
   
   void setSensorName(uint8_t nId, const char* szName) {
      if (szName != nullptr) {
         CxSensor* pSensor = getSensor(nId);
         if (pSensor) {
            pSensor->setName(szName);
         }
      }
   }
   
   const char* getSensorName(uint8_t nId) {
      CxSensor* pSensor = getSensor(nId);
      if (pSensor) {
         return pSensor->getName();
      }
      return nullptr;
   }
   
   /**
    * @brief Print a list of all sensors to the console.
    */
   void printList() {
      /// print table header (ID, name, type, value, unit)
      __console.printf(F(ESC_ATTR_BOLD "ID | Name        | Type            | Model    | Value   | Unit\n" ESC_ATTR_RESET));
      
      /// iterate over all sensors and print formated sensor information
      for (const auto& [nId, pSensor] : _mapSensors) {
         __console.printf(F(ESC_ATTR_BOLD "%02d" ESC_ATTR_RESET " | %-11s | %-15s | %-8s | %7.1f | %s\n"), nId, pSensor->getName(), pSensor->getTypeSz(), pSensor->getModel() ,pSensor->getFloatValue(), pSensor->getUnit());
      }
   }
   
};

void CxSensor::registerSensors() {
   CxSensorManager& manager = CxSensorManager::getInstance();
   manager.addSensor(this);
}

/// Unregister the sensor from the manager
void CxSensor::unregisterSensors() {
   CxSensorManager& manager = CxSensorManager::getInstance();
   manager.removeSensor(this);
}
   

#endif /* CxSensorManager_hpp */
