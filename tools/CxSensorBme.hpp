/**
 * @file CxSensorBme.hpp
 * @brief Defines the CxSensorBme class for managing BME280 sensor capabilities in an embedded system.
 * @details The CxSensorBme class provides methods for managing BME280 sensor capabilities, including initialization, reading sensor data, and updating sensor values.
 *
 * Dependencies:
 * - CxSensorManager.hpp
 * - CxCapabilityI2C.hpp   (for I2C device configuration)
 * - Adafruit_BME280.h     (for BME280 sensor library)
 * - Adafruit_Sensor.h     (for Adafruit sensor library)
 *
 * This file contains the following class:
 * - CxSensorBme: Manages BME280 sensor capabilities, including initialization, reading sensor data, and updating sensor values.
 * - CxBmeSensorContainer: Manages CxSensor objects to represent the three environmental sensors (temperature, humidity and pressure) of the BME280 sensor.
 *
 * The global object of the CxSensorBmeContainer (bmeContainer) class provides access to the individual sensor objects.
 *
 * @date Created by ocfu on 16.03.25.
 * @copyright © 2025 ocfu. All rights reserved.
 *
 * Key Features:
 * 1. Classes and Enumerations:
 *    - CxSensorBme: Extends CxSensor to manage BME280 sensor capabilities.
 *    - CxBmeSensorContainer: Manages a container of the BME environmental sensors.
 *
 * 2. CxSensorBme Class:
 *    - Manages BME280 sensor properties such as type, resolution, name, model, and value.
 *    - Provides methods to enable/disable sensors, set/get sensor values, and update sensor readings.
 *    - Registers and unregisters sensors with the CxSensorManager.
 *
 * 3. CxBmeSensorContainer Class:
 *    - Manages a collection of the BME environemental sensors using a vector.
 *    - Provides methods to initialize, end, and print sensor information.
 *
 * Relationships:
 * - CxSensorBme is a subclass of CxSensor and interacts with CxSensorManager to register/unregister sensors.
 * - CxBmeSensorContainer manages a collection of CxSensorBme objects.
 *
 * How They Work Together:
 * - CxSensorBme represents individual BME280 sensors with specific properties and methods.
 * - CxBmeSensorContainer maintains a collection of these sensors, allowing for centralized management and updates.
 * - Sensors register themselves with the CxSensorManager upon creation and unregister upon destruction.
 * - The CxBmeSensorContainer can initialize, end, and print sensor information.
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
#ifndef CxSensorBme_hpp
#define CxSensorBme_hpp

#include "../capabilities/CxCapabilityI2C.hpp"
#include "CxSensorManager.hpp"

#ifdef ARDUINO
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <Bme280.h>
#endif

// Over/underrun hysteresis checks
#define OVERRUN_H(_x, _v, _th, _ph)  ((_x && (_v > _th * (1.0 - _ph))) || ((_v >= _th)))
#define UNDERRUN_H(_x, _v, _th, _ph) ((_x && (_v < _th)) || ((_v <= _th * (1.0 - _ph))))

/**
 * @class CxSensorBme
 * @brief Manages BME280 sensor capabilities, including initialization, reading sensor data, and updating sensor values.
 * @details The CxSensorBme class extends the CxSensor class and provides methods for managing BME280 sensor capabilities.
 * The class includes methods for initializing the sensor, reading sensor data, and updating sensor values.
 */
class CxSensorBme : public CxSensor {
#ifdef ARDUINO
   //Adafruit_BME280 _bme; /// BME280 sensor object
   Bme280TwoWire _bme; /// BME280 sensor object
#endif
   CxI2CDevice* _pI2CDev = nullptr; /// Pointer to I2C device
   bool _bBme = false; /// Flag to indicate if BME sensor is initialized
   
public:
   CxSensorBme() {}
   CxSensorBme(const char* name) { __strName = name; }
   CxSensorBme(CxI2CDevice* pDev, ECSensorType eType, uint8_t res = 12) { begin(pDev, eType, res); }
   ~CxSensorBme() { unregisterSensors(); } /// Destructor to unregister sensors
   
   /// Initialize the sensor with I2C device, sensor type, and resolution
   bool begin(CxI2CDevice* pDev, ECSensorType eType, uint8_t res = 12) { setI2CDevice(pDev); setType(eType); setResolution(res); return begin(); }
   
   /// Begin sensor initialization
   bool begin() {
      if ((_pI2CDev == nullptr) || (!_pI2CDev->isEnabled())) {
         __console.warn(F("SENS: BME device is not enabled in configuration!"));
         return false;
      }
      
      if (!isEnabled()) {
         __console.warn(F("SENS: BME sensor is not enabled in configuration!"));
         return false;
      }
      
      if (!_bBme && _pI2CDev != nullptr) {
         _CONSOLE_INFO(F("SENS: start new BME sensor at addr %02X"), _pI2CDev->getAddr());
#ifdef ARDUINO
         _bBme = _bme.begin((_pI2CDev->getAddr() == (uint8_t) Bme280TwoWireAddress::Primary) ? Bme280TwoWireAddress::Primary : Bme280TwoWireAddress::Secondary ); /// Initialize BME sensor
         _bme.setSettings(Bme280Settings::indoor());
#endif
      }
      
      if (_bBme) {
#ifdef ARDUINO
         //sensor_t sensor;
#endif
         __bValid = true;
         
         /// Set sensor properties based on the type
         switch (getType()) {
            case ECSensorType::temperature:
#ifdef ARDUINO
               //_bme.getTemperatureSensor()->getSensor(&sensor);
               __fMaxValue = 85.0;
               __fMinValue = -40.0;
               __fResolution = 0.01;
               if (__strName.isEmpty()) {
                  __strName = "temp";
                  __strName += _pI2CDev->getAddr();
               }
               __strModel = F("BME280");
               __strUnit = F("°C");
               __nId = _bme.getChipId();
#endif
               break;
               
            case ECSensorType::humidity:
#ifdef ARDUINO
               //_bme.getHumiditySensor()->getSensor(&sensor);
               __fMaxValue = 100.0;
               __fMinValue = 0.0;
               __fResolution = 0.008;
               if (__strName.isEmpty()) {
                  __strName = "hum";
                  __strName += _pI2CDev->getAddr();
               }
               __strModel = F("BME280");
               __strUnit = "%";
               __nId = _bme.getChipId();
#endif
               break;
               
            case ECSensorType::pressure:
#ifdef ARDUINO
               //_bme.getPressureSensor()->getSensor(&sensor);
               __fMaxValue = 1100;
               __fMinValue = 300;
               __fResolution = 0.18;
               if (__strName.isEmpty()) {
                  __strName = "pres";
                  __strName += _pI2CDev->getAddr();
               }
               __strModel = F("BME280");
               __strUnit = "hPa";
               __nId = _bme.getChipId();
#endif
               break;
               
            default:
               break;
         }
         registerSensors(); /// Register the sensor with the manager
         update(); /// Update sensor readings
      } else {
         __console.error(F("SENS: ### BME begin failed! (addr=%02X)"), (_pI2CDev != nullptr) ? _pI2CDev->getAddr() : 0);
         __bValid = false;
      }
      return __bValid;
   }
   
   /// Get the I2C device
   CxI2CDevice* getI2CDevice() { return _pI2CDev; }
   /// Set the I2C device
   void setI2CDevice(CxI2CDevice* set) { _pI2CDev = set; }
   
   /// Read sensor data
   bool read() {
      if (isValid()) {
         float fValue = 0.0;
         
         /// Read value based on sensor type
         switch (getType()) {
            case ECSensorType::temperature:
#ifdef ARDUINO
               fValue = _bme.getTemperature();
#endif
               break;
            case ECSensorType::humidity:
#ifdef ARDUINO
               fValue = _bme.getHumidity();
#endif
               break;
            case ECSensorType::pressure:
#ifdef ARDUINO
               fValue = _bme.getPressure() / 100.0;
#endif
               break;
            default:
               return false;
               break;
         }
         
         /// Validate and store the sensor value
         if (fValue >= __fMinValue && fValue < __fMaxValue) {
            __fValue = fValue;
            __nValue = round(fValue);
            return true;
         }
         
         /// Restart sensor if reading fails
         static CxTimer60s timer60s;
         if (timer60s.isDue() && _pI2CDev != nullptr) {
            _CONSOLE_INFO(F("SENS: restart BME sensor at addr %02X"), _pI2CDev->getAddr());
#ifdef ARDUINO
            _bBme = _bme.begin((_pI2CDev->getAddr() == (uint8_t) Bme280TwoWireAddress::Primary) ? Bme280TwoWireAddress::Primary : Bme280TwoWireAddress::Secondary ); /// Initialize BME sensor
#endif
            delay(100);
         }
      }
      return false;
   }
   
};


/**
 * @class CxBmeSensorContainer
 * @brief Manages a container of BME sensors.
 * @details The CxBmeSensorContainer class manages a container of BME sensors and provides methods for initialising and ending the sensor manager, getting the number of sensors, and printing sensor information.
 *
 * The class stores a vector of BME sensors of the class CxSensorBme, which is a subclass of the CxSensor class. The CxSensor class registers the sensors in the instance of the CxSensorManager class and makes the sensor available for the ESP console.
 *
 * The class is a singleton and enforces the singleton pattern by disabling copying and assignment. The class is constructed on first access and destroyed when the program ends.
 */
class CxBmeSensorContainer : public CxInitializer {

   /// Vector of BME sensors
   std::vector<std::unique_ptr<CxSensorBme>> _vBmeSensors; /// vector of BME sensors
   
   CxBmeSensorContainer() {VI2CInitializers.push_back(this);} /// register this instance in the vector of initializers. Will be called in the setup() of the I2C capability.

protected:
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();  /// Reference to the console instance
   
public:
   /// Disable copying and assignment to enforce singleton pattern
   CxBmeSensorContainer(const CxBmeSensorContainer&) = delete;
   CxBmeSensorContainer& operator=(const CxBmeSensorContainer&) = delete;
   
   /// Destructor to unregister sensors
   ~CxBmeSensorContainer() {
      end();
   }
   
   /// Singleton pattern
   /// @return Reference to the singleton instance
   /// @details The static instance is constructed on first access
   static CxBmeSensorContainer& getInstance() {
      static CxBmeSensorContainer instance;
      return instance;
   }
   
   /// Begin sensor manager
   /// @details Initialise BME sensors using I2C device
   virtual void init() override {
      CxCapabilityI2C* pI2C = CxCapabilityI2C::getInstance();
      
      if (pI2C) {
         _CONSOLE_DEBUG(F("initialise BME sensors..."));
         /// Creates and add a BME sensors to the vector using std::make_unique for save memory management
         _vBmeSensors.push_back(std::make_unique<CxSensorBme>(pI2C->getBmeDevice(), ECSensorType::temperature));
         _vBmeSensors.push_back(std::make_unique<CxSensorBme>(pI2C->getBmeDevice(), ECSensorType::humidity));
         _vBmeSensors.push_back(std::make_unique<CxSensorBme>(pI2C->getBmeDevice(), ECSensorType::pressure));
      }
      printSensors();
   }
   
   /// End sensor manager
   void end() {
      _vBmeSensors.clear();
   }
   
   /// Print BME sensor information
   void printSensors() {
      _CONSOLE_INFO(F("Registered BME sensors:"));
      for (auto& sensor : _vBmeSensors) {
         __console.printf(F("%s %s %.2f %s\n"), sensor->getName(), sensor->getModel(), sensor->getFloatValue(), sensor->getUnit());
      }
   }
   
};


/**
 * @brief Global BME sensor container instance
 * @details The global BME sensor container instance is created on first access and destroyed when the program ends.
 * The instance is used to manage a container of BME sensors.
 */
CxBmeSensorContainer& bmeContainer = CxBmeSensorContainer::getInstance();
   
   
#endif /* CxSensorBme_hpp */
