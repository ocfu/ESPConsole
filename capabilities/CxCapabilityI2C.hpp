/**
 * @file CxCapabilityI2C.hpp
 * @brief Defines classes for managing I2C devices and capabilities in an embedded system.
 *
 * This file contains the following classes:
 * - CxI2CDevice: Represents an I2C device with properties such as category, type, address, and state.
 * - CxCapabilityI2C: Manages I2C capabilities, including initialization, scanning for devices, and executing commands.
 *
 * The code includes conditional compilation for Arduino-specific libraries and functions.
 *
 * @date Created by ocfu on 09.03.25.
 * @copyright © 2025 ocfu. All rights reserved.
 */

#ifndef CxCapabilityI2C_hpp
#define CxCapabilityI2C_hpp

#include "CxCapability.hpp"
#include "CxESPConsole.hpp"

#include "../capabilities/CxCapabilityFS.hpp"

#include "tools/CxGpioTracker.hpp"
#include "tools/CxTimer.hpp"


#include <map>

#ifdef ARDUINO
#include <Wire.h>
#include <SPI.h>
#endif

class CxCapabilityI2C;
class CxI2CDevice;

tInitializerVector VI2CInitializers;


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
   bool _bEnabled = true; ///< Indicates whether the device is enabled
   bool _bError = false; ///< Indicates whether an error occurred
   
   EI2CDeviceCat _eCat = EI2CDeviceCat::unknown; ///< Category of the device
   EI2CDeviceType _eType = EI2CDeviceType::none; ///< Type of the device
   const char* _szType = ""; ///< Type of the device as a string zero-terminated
   const char* _szCat = ""; ///< Category of the device as a string zero-terminated
   int8_t _nAddr = -1; ///< Address of the device
   char _szAddr[5] = {0}; ///< Address of the device as a string zero-terminated
   
   
   bool _bInit = false; ///< Indicates whether the device is initialized
   
   CxCapabilityI2C* _pI2C = nullptr; ///< Pointer to the I2C capability
   
public:
   /**
    * @brief Default constructor.
    */
   CxI2CDevice() : CxI2CDevice(-1, nullptr) {}
   /**
    * @brief Constructor with address and I2C capability.
    * @param nAddr Address of the device.
    * @param pI2C Pointer to the I2C capability.
    */
   CxI2CDevice(int nAddr, CxCapabilityI2C* pI2C) {setI2C(pI2C);setAddr(nAddr);}
   
   void setEnabled(bool set = true) {_bEnabled = set;}
   bool isEnabled() {return _bEnabled;}
   
   bool isKnown() {return (_eType != EI2CDeviceType::none);}
   bool isInit() {return _bInit;}
   
   void setAddr(int nAddr) {_nAddr=nAddr;setCatByAddr(nAddr);snprintf(_szAddr, sizeof(_szAddr), "%x", _nAddr);_bInit = true;}
   int getAddr() {return _nAddr;}
   const char* getAddrSz() {return _szAddr;}
   const char* getIdSz() {return _szAddr;}
   
   void setError(bool set) {_bError = set;}
   bool hasError() {return _bError;}
   
   void setI2C(CxCapabilityI2C* set) {_pI2C = set;}
   CxCapabilityI2C* getI2C() {return _pI2C;}
   
   void setCat(EI2CDeviceCat eCat) {_eCat = eCat;}
   void setCatByAddr(int nAddr){
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
   
   EI2CDeviceCat getCat() {return _eCat;}
   
   void setType(EI2CDeviceType eType) {_eType = eType;}
   EI2CDeviceType getType() {return _eType;}
   const char* getTypeSz() {return _szType;}
   const char* getCatSz() {return _szCat;}
};

/**
 * @class CxCapabilityI2C
 * @brief Manages I2C capabilities, including initialization, scanning for devices, and executing commands.
 */
class CxCapabilityI2C : public CxCapability {
   /**
    * @var console
    * @brief Reference to the console instance.
    */
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();

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
   /**
    * @brief Default constructor.
    */
   explicit CxCapabilityI2C()
   : CxCapability("i2c", getCmds()), _bEnabled(true), _bRescan(false), _lFreq(100000), _bChanged(false), _bError(false), _bOnline(false), _bBme(false), _bOled(false) {}
   static constexpr const char* getName() { return "i2c"; }
   static const std::vector<const char*>& getCmds() {
      static std::vector<const char*> commands = { "i2c" };
      return commands;
   }
   /**
    * @brief Constructs an I2C capability with the given parameters.
    * @param param The parameters for the I2C capability.
    * @return A unique pointer to the constructed I2C capability.
    */
   static std::unique_ptr<CxCapability> construct(const char* param) {
      return std::make_unique<CxCapabilityI2C>();
   }
   
   /**
    * @brief Destructor.
    */
   ~CxCapabilityI2C() {
      _bEnabled = false;
      for (auto& [addr, device] : _mapDevices) {
         delete device;
      }
      _mapDevices.clear();
   }
   
   /**
    * @brief Initializes the I2C capability.
    */
   void setup() override {
      CxCapability::setup();
      
      setIoStream(*__console.getStream());
      __bLocked = false;
      
      _CONSOLE_INFO(F("====  Cap: %s  ===="), getName());
      
      __console.executeBatch("init", getName());
      
   }
   
   /**
    * @brief Access the singleton instance of CxCapabilityI2C.
    * @return Reference to the singleton instance
    * @note This function is used to access the singleton instance of CxCapabilityI2C.
    */
   static CxCapabilityI2C* getInstance() {
      return static_cast<CxCapabilityI2C*>(ESPConsole.getCapInstance("i2c"));
   }
   
   /**
    * @brief Loops the I2C capability.
    */
   void loop() override {
      if (_bEnabled && _timer60sScan.isDue()) {
         //scan();
      }
   }

   /**
    * @brief Executes a command for the I2C capability.
    * @param szCmd The command to execute.
    * @return True if the command is executed successfully, otherwise false.
    */
   uint8_t execute(const char *szCmd, uint8_t nClient) override {
      // validate the call
      if (!szCmd) return EXIT_FAILURE;
      
      // get the command and arguments into the token buffer
      CxStrToken tkCmd(szCmd, " ");
      
      // we have a command, find the action to take
      String strCmd = TKTOCHAR(tkCmd, 0);
      
      // removes heading and trailing white spaces
      strCmd.trim();
      
      uint8_t nExitValue = EXIT_FAILURE;

      if ((strCmd == "?")) {
         printCommands();
      } else if (strCmd == "i2c") {
         String strSubCmd = TKTOCHAR(tkCmd, 1);
         String strEnv = ".i2c";
         if (strSubCmd == "enable") {
            _bEnabled = (bool)TKTOINT(tkCmd, 2, 0);
            if (_bEnabled) nExitValue = init();
         } else if (strSubCmd == "list") {
            printDevices();
            nExitValue = EXIT_SUCCESS;
         } else if (strSubCmd == "scan") {
            if (_bEnabled) nExitValue = scan();
         } else if (strSubCmd == "setpins" && (tkCmd.count() >= 4)) {
            nExitValue = setPins(TKTOINT(tkCmd, 2, -1), TKTOINT(tkCmd, 3, -1), TKTOINT(tkCmd, 4, -1));
         } else if (strSubCmd == "init") {
            nExitValue = init();
         } else {
            printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), _bEnabled);
            printf(F(ESC_ATTR_BOLD " SDA Pin:      " ESC_ATTR_RESET "%d\n"), _gpioSda.getPin());
            printf(F(ESC_ATTR_BOLD " SCL Pin:      " ESC_ATTR_RESET "%d\n"), _gpioScl.getPin());
            printf(F(ESC_ATTR_BOLD " VU Pin:       " ESC_ATTR_RESET "%d\n"), _gpioVu.getPin());
            __console.man(getName());
         }
      } else {
         // command not handled here
         return EXIT_NOT_HANDLED;
      }
      g_Stack.update();
      return nExitValue;
   }
   
   /**
    * @brief Initializes the I2C capability.
    */
   uint8_t init() {
      if (_bEnabled) {
         // both pins must be ok and different, otherwise disable the service
         if (hasValidPins()) {
            // power on I2C sensor device first
            if (hasValidVuPin()) {
               _CONSOLE_INFO(F("I2C: power on gpio=%d"), _gpioVu.getPin());
               reset();
            }
            _CONSOLE_INFO(F("I2C: begin Wire on sda=%d, scl=%d, clock: %d kHz"), _gpioSda.getPin(), _gpioScl.getPin(), getClock()/1000);
#ifdef ARDUINO
            Wire.setClock(getClock());
            Wire.begin(_gpioSda.getPin(), _gpioScl.getPin());
#endif
            scan();
            
            // loop through all initializers
            for (auto& pInit : VI2CInitializers) {
               if (pInit) {
                  pInit->init();
               }
            }
            return EXIT_SUCCESS;
         }
      }
      return EXIT_FAILURE;
   }


   /**
    * @brief Scans for I2C devices.
    */
   CxI2CDevice* findDevice(int nAddr) {
      tI2CDeviceMap::iterator it = _mapDevices.begin();
      while(it != _mapDevices.end())
      {
         if(it->second->getAddr() == nAddr) {
            return it->second;
         }
         it++;
      }
      return nullptr;
   }

   /**
    * @brief Lists all devices in the map.
    */
   void printDevices() {
      CxTablePrinter table(getIoStream());
      
      table.printHeader({F("Addr"), F("Type"), F("Category")}, {4, 10, 20});
      
      for (const auto& [address, device] : _mapDevices) {
         table.printRow({String(address, 16).c_str(), device->getTypeSz(), device->getCatSz()});
      }
   }
   
   /**
    * @brief Gets the OLED device.
    * @return The OLED device if found, otherwise nullptr.
    */
   CxI2CDevice* getOledDevice() {return findDevice(getOledAddr());}
   /**
    * @brief Gets the BME device.
    * @return The BME device if found, otherwise nullptr.
    */
   CxI2CDevice* getBmeDevice() {return findDevice(getBmeAddr());}
   
   bool hasValidPins() {return (_gpioSda.isValid() && _gpioScl.isValid() && _gpioSda.getPin() != _gpioScl.getPin());}
   bool hasValidVuPin() {return _gpioVu.isValid();}
   bool hasBme() {return _bBme;}
   bool hasOled() {return _bOled;}
   
   bool hasChanged() {return _bChanged;}
   bool hasError() {return _bError;}
   
   /**
    * @brief Powers off the I2C device when VU pin is valid.
    */
   void powerOff() {if (hasValidVuPin()) _gpioVu.setLow();}
   /**
    * @brief Powers on the I2C device when VU pin is valid.
    */
   void powerOn() {if (hasValidVuPin()) _gpioVu.setHigh();}
   /**
    * @brief Resets the I2C device.
    */
   void reset() {; powerOff();delay(100);powerOn();}
   
   /**
    * @brief Gets the address of a device by type.
    * @param type The type of the device.
    * @return The address of the device if found, otherwise -1.
    */
   int getDeviceAddr(CxI2CDevice::EI2CDeviceType type) {
      for (const auto& [addr, device] : getDeviceMap()) {
         if (device && device->getType() == type) {
            return device->getAddr();
         }
      }
      return -1;
   }
   /**
    * @brief Gets the address of the OLED device.
    * @return The address of the OLED device if found, otherwise -1.
    */
   int getOledAddr() {
      return getDeviceAddr(CxI2CDevice::EI2CDeviceType::oled);
   }
   
   /**
    * @brief Gets the address of the BME device.
    * @return The address of the BME device if found, otherwise -1.
    */
   int getBmeAddr() {
      return getDeviceAddr(CxI2CDevice::EI2CDeviceType::bme);
   }
   
   /**
    * @brief Gets the SDA, SCL and VU GPIOs.
    * @return The GPIO.
    */
   CxGPIO& getGPIOSda() {return _gpioSda;}
   CxGPIO& getGPIOScl() {return _gpioScl;}
   CxGPIO& getGPIOVu() {return _gpioVu;}

   /**
    * @brief Sets the I2C clock frequency.
    * @param lFreq The clock frequency.
    */
   void setClock(unsigned long lFreq) {_lFreq = lFreq;}
   unsigned long getClock() {return _lFreq;}
   
   void setRescan(bool set) {_bRescan = set;}
   bool isRescan() {return _bRescan;}

   /**
    * @brief Sets the SDA, SCL and VU pins.
    * @param sda The SDA pin.
    * @param scl The SCL pin.
    * @param vu The VU pin.
    */
   uint8_t setPins(int sda, int scl, int vu) {
      _CONSOLE_DEBUG(F("CI2C: setPins(sda=%d, scl=%d, vu=%d)"), sda, scl, vu);
      _gpioSda.setPin(sda);
      _gpioSda.setGpioName("sda");
      _gpioScl.setPin(scl);
      _gpioScl.setGpioName("scl");
      _gpioVu.setPin(vu);
      _gpioVu.setGpioName("vu"); // this pin powers the BME280 to be able to restart the sensor, if data fails
      _gpioVu.setHigh(); // power on
      return hasValidPins() ? EXIT_SUCCESS : EXIT_FAILURE;
      
   }
   /**
    * @brief Scans for I2C devices with a specified frequency.
    * @param lFreq The frequency to scan.
    */
   uint8_t scan(unsigned long lFreq) {
      _CONSOLE_INFO(F("I2C: start scan with freq = %d kHz..."), lFreq/1000);
      
      int  nError = -1;
      _bError = false;
      _bChanged = false;
      _bOnline = true;
      
      uint8_t nExitValue = EXIT_FAILURE;
      
      /// scan all I2C addresses
      for (int i=1; i<128; i++) {
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
               _CONSOLE_INFO(F("I2C: found Device at 0x%02X (%s) at freq %d kHz"), i, pDev->getTypeSz(), lFreq/1000);
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
      Wire.setClock(getClock()); // reset to configured clock speed
#endif
      return EXIT_SUCCESS;
   }
   
   /**
    * @brief Scans for I2C devices with both frequencies.
    */
   uint8_t scan() {scan(100000);return scan(400000);}
   
   /**
    * @brief Gets the map of I2C devices.
    * @return The map of I2C devices.
    */
   tI2CDeviceMap& getDeviceMap() {return _mapDevices;}

   static void loadCap() {
      CAPREG(CxCapabilityI2C);
      CAPLOAD(CxCapabilityI2C);
   };
};


#endif /* CxCapabilityI2C_hpp */

