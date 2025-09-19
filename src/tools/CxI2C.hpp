
#ifndef CxI2C_hpp
#define CxI2C_hpp

#include <map>

#include "CxGpioDeviceManager.hpp"
#include "CxTimer.hpp"

#ifdef ARDUINO
#include <SPI.h>
#include <Wire.h>
#endif

class CxI2CDevice;
class CxI2CManager;

extern CxI2CManager _i2cManager;
extern tInitializerVector VI2CInitializers;

typedef std::map<int, CxI2CDevice*> tI2CDeviceMap;

/**
 * @class CxI2CDevice
 * @brief Represents an I2C device with properties such as category, type, address, and state.
 */
class CxI2CDevice {
  public:
   /**
    * @enum EI2CDeviceCat
    * @brief Enumerates the categories of I2C devices.
    */
   enum class EI2CDeviceCat {
      unknown = 0,
      uC,
      sensor,
      expander,
      display,
      adc,
      eprom,
      fram,
      dac,
      rtc,
      led,
      mux,
      segdisp
   };

   /**
    * @enum EI2CDeviceType
    * @brief Enumerates the types of I2C devices.
    */
   enum class EI2CDeviceType {
      none = 0,
      bme,
      oled
   };

  private:
   bool _bEnabled = true;  ///< Indicates whether the device is enabled
   bool _bError = false;   ///< Indicates whether an error occurred

   EI2CDeviceCat _eCat = EI2CDeviceCat::unknown;  ///< Category of the device
   EI2CDeviceType _eType = EI2CDeviceType::none;  ///< Type of the device
   const char* _szType = "";                      ///< Type of the device as a string zero-terminated
   const char* _szCat = "";                       ///< Category of the device as a string zero-terminated
   int8_t _nAddr = -1;                            ///< Address of the device
   char _szAddr[5] = {0};                         ///< Address of the device as a string zero-terminated

   bool _bInit = false;  ///< Indicates whether the device is initialized

   CxI2CManager* _pManager = nullptr;  ///< Pointer to the I2C manager

  public:
   /**
    * @brief Default constructor.
    */
   CxI2CDevice() : CxI2CDevice(-1, nullptr) {}
   /**
    * @brief Constructor with address.
    * @param nAddr Address of the device.
    */
   CxI2CDevice(int nAddr, CxI2CManager* pManager) {
      setAddr(nAddr);
      _pManager = pManager;
   }

   void setEnabled(bool set = true) { _bEnabled = set; }
   bool isEnabled() { return _bEnabled; }

   bool isKnown() { return (_eType != EI2CDeviceType::none); }
   bool isInit() { return _bInit; }

   void setAddr(int nAddr) {
      _nAddr = nAddr;
      setCatByAddr(nAddr);
      snprintf(_szAddr, sizeof(_szAddr), "%x", _nAddr);
      _bInit = true;
   }
   int getAddr() { return _nAddr; }
   const char* getAddrSz() { return _szAddr; }
   const char* getIdSz() { return _szAddr; }

   void setError(bool set) { _bError = set; }
   bool hasError() { return _bError; }

   void setCat(EI2CDeviceCat eCat) { _eCat = eCat; }
   void setCatByAddr(int nAddr) {
      switch (nAddr) {
         case 0x20:
         case 0x21:
         case 0x22:
         case 0x23:
         case 0x24:
         case 0x25:
         case 0x26:
         case 0x27:
            setCat(CxI2CDevice::EI2CDeviceCat::expander);
            _szType = "MCP23017,MCP23S17,PCF8574N,PCF8574P";
            _szCat = "Expander";
            break;
         case 0x38:
         case 0x39:
         case 0x3A:
         case 0x3B:
         case 0x3D:
         case 0x3E:
         case 0x3F:
            setCat(CxI2CDevice::EI2CDeviceCat::expander);
            _szType = "PCF8574T/AT/AN";
            _szCat = "Expander";
            break;
         case 0x3C:
            setCat(CxI2CDevice::EI2CDeviceCat::display);
            _szType = "OLED";
            setType(EI2CDeviceType::oled);
            _szCat = "Display";
            break;

         case 0x76:
         case 0x77:
            setCat(CxI2CDevice::EI2CDeviceCat::sensor);
            _szType = "BME280";
            setType(EI2CDeviceType::bme);
            _szCat = "Sensor";
            break;
         default:
            setCat(CxI2CDevice::EI2CDeviceCat::unknown);
            _szType = "";
            break;
      }
   }

   EI2CDeviceCat getCat() { return _eCat; }

   void setType(EI2CDeviceType eType) { _eType = eType; }
   EI2CDeviceType getType() { return _eType; }
   const char* getTypeSz() { return _szType; }
   const char* getCatSz() { return _szCat; }
};

class CxI2CManager {
   /**
    * @var console
    * @brief Reference to the console instance.
    */

   bool _bEnabled = true;

   /// GPIOs for I2C
   CxGPIO _gpioSda;
   CxGPIO _gpioScl;
   CxGPIO _gpioVu;

   /// I2C Timer
   CxTimer60s _timer60sScan;

   bool _bRescan = false;

   unsigned long _lFreq = 100000;

   bool _bChanged = false;
   bool _bError = false;
   bool _bOnline = false;

   /// Map of I2C devices
   tI2CDeviceMap _mapDevices;

   bool _bBme = false;
   bool _bOled = false;

  public:
   CxI2CManager() {}

   ~CxI2CManager() {
      for (auto& [addr, device] : _mapDevices) {
         delete device;
      }
      _mapDevices.clear();
   }

   void setEnabled(bool set = true) { _bEnabled = set; }
   bool isEnabled() { return _bEnabled; }

   void setup() {
      __console.executeBatch("init", "i2c");
   }

   void loop() {
      if (_bEnabled && _timer60sScan.isDue()) {
         // scan();
      }
   }

   uint8_t init() {
      _bEnabled = true;
      // both pins must be ok and different, otherwise disable the service
      if (hasValidPins()) {
         // power on I2C sensor device first
         if (hasValidVuPin()) {
            _CONSOLE_INFO(F("I2C: power on gpio=%d"), _gpioVu.getPin());
            reset();
         }
         _CONSOLE_INFO(F("I2C: begin Wire on sda=%d, scl=%d, clock: %d kHz"), _gpioSda.getPin(), _gpioScl.getPin(), getClock() / 1000);
#ifdef ARDUINO
         Wire.setClock(getClock());
         Wire.begin(_gpioSda.getPin(), _gpioScl.getPin());
#endif
         scan();

         // loop through all initializers
         for (auto& pInit : VI2CInitializers) {
            if (pInit) {
               _CONSOLE_INFO(F("I2C: initializing bme container"));
               pInit->init();
            }
         }
         return EXIT_SUCCESS;
      } else {
         _bEnabled = false;
      }
      return EXIT_FAILURE;
   }

   CxI2CDevice* findDevice(int nAddr) {
      tI2CDeviceMap::iterator it = _mapDevices.begin();
      while (it != _mapDevices.end()) {
         if (it->second->getAddr() == nAddr) {
            return it->second;
         }
         it++;
      }
      return nullptr;
   }

   void printDevices() {
      CxTablePrinter table(getIoStream());

      table.printHeader({F("Addr"), F("Type"), F("Category")}, {4, 10, 20});

      for (const auto& [address, device] : _mapDevices) {
         table.printRow({String(address, 16).c_str(), device->getTypeSz(), device->getCatSz()});
      }
   }

   CxI2CDevice* getOledDevice() { return findDevice(getOledAddr()); }
   CxI2CDevice* getBmeDevice() { return findDevice(getBmeAddr()); }

   bool hasValidPins() { return (_gpioSda.isValid() && _gpioScl.isValid() && _gpioSda.getPin() != _gpioScl.getPin()); }
   bool hasValidVuPin() { return _gpioVu.isValid(); }
   bool hasBme() { return _bBme; }
   bool hasOled() { return _bOled; }

   bool hasChanged() { return _bChanged; }
   bool hasError() { return _bError; }

   void powerOff() {
      if (hasValidVuPin()) _gpioVu.setLow();
   }
   void powerOn() {
      if (hasValidVuPin()) _gpioVu.setHigh();
   }
   void reset() {
      powerOff();
      delay(100);
      powerOn();
   }

   int getDeviceAddr(CxI2CDevice::EI2CDeviceType type) {
      for (const auto& [addr, device] : getDeviceMap()) {
         if (device && device->getType() == type) {
            return device->getAddr();
         }
      }
      return -1;
   }

   int getOledAddr() {
      return getDeviceAddr(CxI2CDevice::EI2CDeviceType::oled);
   }

   int getBmeAddr() {
      return getDeviceAddr(CxI2CDevice::EI2CDeviceType::bme);
   }

   CxGPIO& getGPIOSda() { return _gpioSda; }
   CxGPIO& getGPIOScl() { return _gpioScl; }
   CxGPIO& getGPIOVu() { return _gpioVu; }

   void setClock(unsigned long lFreq) { _lFreq = lFreq; }
   unsigned long getClock() { return _lFreq; }

   void setRescan(bool set) { _bRescan = set; }
   bool isRescan() { return _bRescan; }

   uint8_t setPins(int sda, int scl, int vu) {
      _CONSOLE_DEBUG(F("CI2C: setPins(sda=%d, scl=%d, vu=%d)"), sda, scl, vu);
      _gpioSda.setPin(sda);
      _gpioSda.setGpioName("sda");
      _gpioScl.setPin(scl);
      _gpioScl.setGpioName("scl");
      _gpioVu.setPin(vu);
      _gpioVu.setGpioName("vu");  // this pin powers the BME280 to be able to restart the sensor, if data fails
      _gpioVu.setHigh();          // power on
      return hasValidPins() ? EXIT_SUCCESS : EXIT_FAILURE;
   }

   uint8_t scan(unsigned long lFreq) {
      _CONSOLE_INFO(F("I2C: start scan with freq = %d kHz..."), lFreq / 1000);

      int nError = -1;
      _bError = false;
      _bChanged = false;
      _bOnline = true;

      uint8_t nExitValue = EXIT_FAILURE;

      /// scan all I2C addresses
      for (int i = 1; i < 128; i++) {
#ifdef ARDUINO
         Wire.setClock(lFreq);
         Wire.beginTransmission(i);
         nError = Wire.endTransmission();
         /**
          * 0: success
          * 1: data too long to fit in transmit buffer
          * 2: received NACK on transmit of address
          * 3: received NACK on transmit of data
          * 4: other error
          */
#endif
         /// find device by address, if not found create new device
         CxI2CDevice* pDev = findDevice(i);

         if (nError == 0) {
            if (pDev) {
               _bChanged = true;
            } else {
               // new device
               pDev = new CxI2CDevice(i, this);
               _mapDevices[i] = pDev;
            }

            if (pDev) {
               _CONSOLE_INFO(F("I2C: found Device at 0x%02X (%s) at freq %d kHz"), i, pDev->getTypeSz(), lFreq / 1000);
               if (pDev->getType() == CxI2CDevice::EI2CDeviceType::bme) {
                  _bBme = true;
               }
               if (pDev->getType() == CxI2CDevice::EI2CDeviceType::oled) {
                  _bOled = true;
               }
               pDev->setError(false);
               nExitValue = EXIT_SUCCESS;
            }
         } else if (nError == 4) {
            _bError = true;
            _bChanged = true;
            if (i == 1) {
               __console.error(F("I2C: ### general bus error"));
               _bOnline = false;
               break;
            } else {
               __console.error(F("I2C: ### error 4 at address %02X"), i);
               if (pDev) pDev->setError(true);
            }
         } else if (pDev) {
            _bError = true;
            _bChanged = true;
            __console.error(F("I2C: ### lost Device at 0x%02X (error %d)"), i, nError);
            pDev->setError(true);
         }
      }
#ifdef ARDUINO
      Wire.setClock(getClock());  // reset to configured clock speed
#endif
      return nExitValue;
   }

   uint8_t scan() {
      scan(100000);
      return scan(400000);
   }

   tI2CDeviceMap& getDeviceMap() { return _mapDevices; }
};
#endif  // CxI2C_hpp
