/**
 * @file CxSensorBme.hpp
 * @brief Defines the CxSensorBme class for managing BME280 sensor capabilities in an embedded system.
 *
 * This file contains the following class:
 * - CxSensorBme: Manages BME280 sensor capabilities, including initialization, reading sensor data, and updating sensor values.
 *
 * The code includes conditional compilation for Arduino-specific libraries and functions.
 *
 * @date Created by ocfu on 31.07.22.
 * @copyright © 2022 ocfu. All rights reserved.
 */
#ifndef CxSensorBme_hpp
#define CxSensorBme_hpp

#include "../capabilities/CxCapabilityI2C.hpp"
#include "CxSensorManager.hpp"

#ifdef ARDUINO
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
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
   Adafruit_BME280 _bme; /// BME280 sensor object
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
         __console.info(F("SENS: start new BME sensor at addr %02X"), _pI2CDev->getAddr());
#ifdef ARDUINO
         _bBme = _bme.begin(_pI2CDev->getAddr()); /// Initialize BME sensor
#endif
      }
      
      if (_bBme) {
#ifdef ARDUINO
         sensor_t sensor;
#endif
         __bValid = true;
         
         /// Set sensor properties based on the type
         switch (getType()) {
            case ECSensorType::temperature:
#ifdef ARDUINO
               _bme.getTemperatureSensor()->getSensor(&sensor);
               __fMaxValue = sensor.max_value;
               __fMinValue = sensor.min_value;
               __fResolution = sensor.resolution;
               if (__strName.isEmpty()) {
                  __strName = "temp";
                  __strName += _pI2CDev->getAddr();
               }
               __strModel = sensor.name;
               __strUnit = "°C";
               __nId = sensor.sensor_id;
#endif
               break;
               
            case ECSensorType::humidity:
#ifdef ARDUINO
               _bme.getHumiditySensor()->getSensor(&sensor);
               __fMaxValue = sensor.max_value;
               __fMinValue = sensor.min_value;
               __fResolution = sensor.resolution;
               if (__strName.isEmpty()) {
                  __strName = "hum";
                  __strName += _pI2CDev->getAddr();
               }
               __strModel = sensor.name;
               __strUnit = "%";
               __nId = sensor.sensor_id;
#endif
               break;
               
            case ECSensorType::pressure:
#ifdef ARDUINO
               _bme.getPressureSensor()->getSensor(&sensor);
               __fMaxValue = sensor.max_value;
               __fMinValue = sensor.min_value;
               __fResolution = sensor.resolution;
               if (__strName.isEmpty()) {
                  __strName = "pres";
                  __strName += _pI2CDev->getAddr();
               }
               __strModel = sensor.name;
               __strUnit = "hPa";
               __nId = sensor.sensor_id;
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
               fValue = _bme.readTemperature();
#endif
               break;
            case ECSensorType::humidity:
#ifdef ARDUINO
               fValue = _bme.readHumidity();
#endif
               break;
            case ECSensorType::pressure:
#ifdef ARDUINO
               fValue = _bme.readPressure() / 100.0;
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
            __console.info(F("SENS: restart BME sensor at addr %02X"), _pI2CDev->getAddr());
#ifdef ARDUINO
            _bBme = _bme.begin(_pI2CDev->getAddr());
#endif
            delay(100);
         }
      }
      return false;
   }
   
};

#endif /* CxSensorBme_hpp */
