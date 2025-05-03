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

#include <map>

/**
 * @brief Enumeration of sensor types.
 */
enum class ECSensorType {
   none = 0,
   temperature,
   humidity,
   pressure,
   other
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
   String _strType;
   
   CxTimer _timer;

protected:
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();  /// Reference to the console instance
   
   unsigned long __nLastUpdate = 0;  /// Timestamp of the last update
   uint8_t __nResolution = 12;  /// Resolution of the sensor (9-12 bits)
   unsigned long __nTimeToConvert = 100;  /// Time required for the sensor to convert/read the measurement
   
   String __strName;  /// Name of the sensor
   String __strFriendlyName;
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
   CxSensor() {startTimer(1000);}  // set default timer rate 1s. 
   virtual ~CxSensor() {end();}
   
   /// Pure virtual method to initialize the sensor
   virtual bool begin() = 0;
   void end() {
      __bValid = false;
      __bValidValue = false;
   }
   
   void startTimer(uint32_t period) {_timer.start(period);}
   bool isDue() {return _timer.isDue();}
   
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
   void setType(ECSensorType eType) {
      _eType = eType;
      switch (_eType) {
         case ECSensorType::temperature: _strType = "temperature"; break;
         case ECSensorType::humidity: _strType =  "humidity"; break;
         case ECSensorType::pressure: _strType = "pressure"; break;
         default: _strType = "other";
      }
   }
   
   /// Get the type of the sensor as a string
   const char* getTypeSz() {
      return _strType.c_str();
   }
   
   void setTypeSz(const char* set) {_strType = set;}
   
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

   void setFriendlyName(const char* name) {if (name) __strFriendlyName = name;}
   const char* getFriendlyName() {
      if (__strFriendlyName.length()) {
         return __strFriendlyName.c_str();
      } else {
         return getName();
      }
   }
   
   void setName(const char* name) {
      if (name) {
         __strName = __console.makeNameIdStr(name);
      }
   }
   const char* getName() {return __strName.c_str();}

   /// Pure virtual method to read sensor values
   virtual bool read() = 0;
   /// Update the sensor value
   bool update(){
      
      if (_timer.isDue()) {
         __fValue = INVALID_FLOAT;
         
         if (isValid()) {
            __bValidValue = read();
            if (!__bValidValue) {
               _CONSOLE_DEBUG_EXT(DEBUG_FLAG_SENSOR, F("SENS: %s (%d) value is not ok"), getName(), getId());
            }
         } else {
            return false;
         }
         
         __nValue = static_cast<uint32_t>(round(__fValue));
         
         return true;
      }
      return false;
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
   std::function<float()> _cb;
   
public:
      CxSensorGeneric(const char* szName, ECSensorType eType, const char* unit = "", std::function<float()> cb = nullptr) : _cb(cb) {
      setType(eType);
      setResolution(12);
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
         
         /// Set the unique ID for the sensor
         pSensor->setId(nId);
         
         /// Add the sensor to the map
         _mapSensors[nId] = pSensor;
      }
   }
   
   /**
    * @brief Remove a sensor from the manager.
    * @param pSensor A pointer to the sensor to remove.
    */
   void removeSensor(CxSensor* pSensor) {
      if (pSensor) {
         for (auto it = _mapSensors.begin(); it != _mapSensors.end(); ++it) {
            if (it->second == pSensor) {
               _mapSensors.erase(it);
               break;
            }
         }
      }
   }
   
   void removeSensor(const char* szName) {
      removeSensor(getSensor(szName));
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
      CxTablePrinter table(*__console.getStream());
      
      table.printHeader({F("Id"), F("Name"), F("Type"), F("Model"), F("Value"), F("Unit")}, {2,11,15,8,8,8});

      /// iterate over all sensors and print formated sensor information
      for (const auto& [nId, pSensor] : _mapSensors) {
         table.printRow({String(nId).c_str(), pSensor->getName(), pSensor->getTypeSz(), pSensor->getModel() , String(pSensor->getFloatValue()).c_str(), pSensor->getUnit()});
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
