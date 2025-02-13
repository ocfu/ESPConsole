//
//  CxI2CDevice.hpp
//
//  Created by ocfu on 31.07.22.
//  Copyright © 2022 ocfu. All rights reserved.
//

#ifndef CxI2CDevice_hpp
#define CxI2CDevice_hpp

#include "CxGpioTracker.hpp"
#include "CxConfigParser.hpp"


#include <map>

#ifdef ARDUINO
#include <Wire.h>
#include <SPI.h>
#endif

class CxI2C;
class CxI2CDevice;

typedef std::map<int, CxI2CDevice*> tI2CDeviceMap;

class CxI2CDevice {
   
public:
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
   
   enum class EI2CDeviceType {
      none = 0,
      bme,
      oled
   };
   
private:
   bool m_bEnabled = true;
   bool m_bError = false;
   
   EI2CDeviceCat m_eCat = EI2CDeviceCat::unknown;
   EI2CDeviceType m_eType = EI2CDeviceType::none;
   const char* m_szType = "";
   const char* m_szCat = "";
   int8_t m_nAddr = -1;
   char m_szAddr[5] = {0};

   
   bool m_bInit = false;
   
   CxI2C* m_pI2C = nullptr;
   
public:
   CxI2CDevice() : CxI2CDevice(-1, nullptr) {invalidate();}
   CxI2CDevice(int nAddr, CxI2C* pI2C) {setI2C(pI2C);setAddr(nAddr);}
   
   void setEnabled(bool set = true) {m_bEnabled = set;}
   bool isEnabled() {return m_bEnabled;}
   
   void invalidate();
   bool isKnown() {return (m_eType != EI2CDeviceType::none);}
   bool isInit() {return m_bInit;}
   
   void setAddr(int nAddr) {m_nAddr=nAddr;setCatByAddr(nAddr);snprintf(m_szAddr, sizeof(m_szAddr), "%x", m_nAddr);m_bInit = true;}
   int getAddr() {return m_nAddr;}
   const char* getAddrSz() {return m_szAddr;}
   const char* getIdSz() {return m_szAddr;}

   void setError(bool set) {m_bError = set;}
   bool hasError() {return m_bError;}
   
   void setI2C(CxI2C* set) {m_pI2C = set;}
   CxI2C* getI2C() {return m_pI2C;}
   
   void setCat(EI2CDeviceCat eCat) {m_eCat = eCat;}
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
            m_szType = "MCP23017,MCP23S17,PCF8574N,PCF8574P";
            m_szCat = "Expander";
            break;
         case 0x38:
         case 0x39:
         case 0x3A:
         case 0x3B:
         case 0x3D:
         case 0x3E:
         case 0x3F:
            setCat(CxI2CDevice::EI2CDeviceCat::expander);
            m_szType = "PCF8574T/AT/AN";
            m_szCat = "Expander";
            break;
         case 0x3C:
            setCat(CxI2CDevice::EI2CDeviceCat::display);
            m_szType = "OLED";
            setType(EI2CDeviceType::oled);
            m_szCat = "Display";
            break;
            
         case 0x76:
         case 0x77:
            setCat(CxI2CDevice::EI2CDeviceCat::sensor);
            m_szType = "BME280";
            setType(EI2CDeviceType::bme);
            m_szCat = "Sensor";
            break;
         default:
            setCat(CxI2CDevice::EI2CDeviceCat::unknown);
            m_szType = "";
            break;
      }
   }
   
   EI2CDeviceCat getCat() {return m_eCat;}
   
   void setType(EI2CDeviceType eType) {m_eType = eType;}
   EI2CDeviceType getType() {return m_eType;}
   const char* getTypeSz() {return m_szType;}
   const char* getCatSz() {return m_szCat;}
};

class CxI2C : public CxESPConsoleBase {
   
   CxESPConsole* _consoleInstance = static_cast<CxESPConsole*>(CxESPConsole::getInstance());
   
   bool m_bEnabled = true;
   
   CxGPIO m_gpioSda;
   CxGPIO m_gpioScl;
   CxGPIO m_gpioVu;
   
   unsigned long m_nTimer1s = 0;
   unsigned long m_nTimer10s = 0;
   unsigned long m_nTimer60s = 0;
   
   bool m_bRescan = false;
   
   unsigned long m_lFreq = 100000;
   
   bool m_bChanged = false;
   bool m_bError = false;
   bool m_bOnline = false;
   
   tI2CDeviceMap m_mapDevices;
   
   bool m_bBme = false;
   bool m_bOled = false;
   
   CxTimer60s _timer60sScan;
   
   bool _processCommand(const char *szCmd, bool bQuiet = false) {
      // validate the call
      if (!szCmd) return false;
      
      // get the command and arguments into the token buffer
      CxStrToken tkCmd(szCmd, " ");
      
      // validate again
      if (!tkCmd.count()) return false;
      
      // we have a command, find the action to take
      String strCmd = TKTOCHAR(tkCmd, 0);
      
      // removes heading and trailing white spaces
      strCmd.trim();
      
      // expect sz parameter, invalid is nullptr
      const char* a = TKTOCHAR(tkCmd, 1);
      const char* b = TKTOCHAR(tkCmd, 2);
      
      if (strCmd == "i2c") {
         String strSubCmd = TKTOCHAR(tkCmd, 1);
         String strEnv = ".i2c";
         if (strSubCmd == "enable") {
            m_bEnabled = (bool)TKTOINT(tkCmd, 2, 0);
            if (m_bEnabled) init();
         } else if (strSubCmd == "list") {
            printDevices();
         } else if (strSubCmd == "scan") {
            if (m_bEnabled) scan();
         }else if (strSubCmd == "save") {
            CxConfigParser Config;
            Config.addVariable("enabled", m_bEnabled);
            Config.addVariable("sda", m_gpioSda.getPin());
            Config.addVariable("scl", m_gpioScl.getPin());
            Config.addVariable("vu", m_gpioVu.getPin());
            if (_consoleInstance) _consoleInstance->saveEnv(strEnv, Config.getConfigStr());
         } else if (strSubCmd == "load") {
            if (_consoleInstance) {
               String strValue;
               if (_consoleInstance->loadEnv(strEnv, strValue)) {
                  CxConfigParser Config(strValue);
                  // extract settings and set, if defined. Keep unchanged, if not set.
                  m_bEnabled = Config.getBool("enabled", m_bEnabled);
                  m_gpioSda.setPin(Config.getInt("sda", m_gpioSda.getPin()));
                  m_gpioScl.setPin(Config.getInt("scl", m_gpioScl.getPin()));
                  m_gpioVu.setPin(Config.getInt("vu", m_gpioVu.getPin()));
               }
            }
         } else {
            printf(F(ESC_ATTR_BOLD " Enabled:      " ESC_ATTR_RESET "%d\n"), m_bEnabled);
            println(F("i2c commands:"));
            println(F("  enable 0|1"));
            println(F("  list"));
            println(F("  scan"));
            println(F("  save"));
            println(F("  load"));
         }
      } else {
         // command not handled here
         return false;
      }
      return true;
   }


public:
   CxI2C() : CxI2C(-1,-1,-1){}
   CxI2C(int sda, int scl, int vu = -1) {
      
      commandHandler.registerCommandSet(F("I2C"), [this](const char* cmd, bool bQuiet)->bool {return _processCommand(cmd, bQuiet);}, F("i2c"), F("I2C commands"));

      setPins(sda, scl, vu);
   }
   
   bool begin() {
      _processCommand("i2c load");
      _CONSOLE_DEBUG(F("start I2C%s"), m_bEnabled ? "" : " (on standby)");
      init();
      return true;
   };
   
   bool begin(int sda, int scl, int vu = -1) {setPins(sda, scl, vu); return begin();}
   void end() {}; // TODO: something to clean up?
   
   void setEnabled(bool set = true) {m_bEnabled = set;}
   bool isEnabled() {return m_bEnabled;}
   bool isOnline() {return m_bOnline;}
   
   void init() {
      if (m_bEnabled) {
         // both pins must be ok and different, otherwise disable the service
         if (hasValidPins()) {
            // power on I2C sensor device first
            if (hasValidVuPin()) {
               _CONSOLE_DEBUG(F("I2C: power on gpio=%d"), m_gpioVu.getPin());
               reset();
            }
            _CONSOLE_DEBUG(F("I2C: begin Wire on sda=%d, scl=%d, clock: %d kHz"), m_gpioSda.getPin(), m_gpioScl.getPin(), getClock()/1000);
#ifdef ARDUINO
            Wire.setClock(getClock());
            Wire.begin(m_gpioSda.getPin(), m_gpioScl.getPin());
#endif
            scan();
         }
      }
   }
      
   void loop() {
      if (m_bEnabled && _timer60sScan.isDue()) {
         //scan();
      }
   };
   
   CxI2CDevice* findDevice(int nAddr) {
      tI2CDeviceMap::iterator it = m_mapDevices.begin();
      while(it != m_mapDevices.end())
      {
         if(it->second->getAddr() == nAddr) {
            return it->second;
         }
         it++;
      }
      return nullptr;
   }
   
   // Function to list all devices in the map
   void printDevices() {
      if (m_mapDevices.empty()) {
         println(F("No devices found in the map."));
         return;
      }
      
      println(F(ESC_ATTR_BOLD "I2C Devices: " ESC_ATTR_RESET));
      for (const auto& [address, device] : m_mapDevices) {
         printf(ESC_TEXT_WHITE " 0x%02x " ESC_ATTR_RESET, address);
         printf("%s (%s)\n", device->getTypeSz(), device->getCatSz());
      }
   }

   
   CxI2CDevice* getOledDevice() {return findDevice(getOledAddr());}
   CxI2CDevice* getBmeDevice() {return findDevice(getBmeAddr());}
   
   bool hasValidPins() {return (m_gpioSda.isValid() && m_gpioScl.isValid() && m_gpioSda.getPin() != m_gpioScl.getPin());}
   bool hasValidVuPin() {return m_gpioVu.isValid();}
   bool hasBme() {return m_bBme;}
   bool hasOled() {return m_bOled;}
   
   bool hasChanged() {return m_bChanged;}
   bool hasError() {return m_bError;}
   
   void powerOff() {if (hasValidVuPin()) m_gpioVu.setLow();}
   void powerOn() {if (hasValidVuPin()) m_gpioVu.setHigh();}
   void reset() {; powerOff();delay(100);powerOn();}
   
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
   
   CxGPIO& getGPIOSda() {return m_gpioSda;}
   CxGPIO& getGPIOScl() {return m_gpioScl;}
   CxGPIO& getGPIOVu() {return m_gpioVu;}
      
   void setClock(unsigned long lFreq) {m_lFreq = lFreq;}
   unsigned long getClock() {return m_lFreq;}
   
   void setRescan(bool set) {m_bRescan = set;}
   bool isRescan() {return m_bRescan;}
   
   void setPins(int sda, int scl, int vu) {
      _CONSOLE_DEBUG(F("CI2C: setPins(sda=%d, scl=%d, vu=%d)"), sda, scl, vu);
      m_gpioSda.setPin(sda);
      m_gpioSda.setName("sda");
      m_gpioScl.setPin(scl);
      m_gpioScl.setName("scl");
      m_gpioVu.setPin(vu);
      m_gpioVu.setName("vu"); // this pin powers the BME280 to be able to restart the sensor, if data fails
      m_gpioVu.setHigh(); // power on
      
   }
   
   void scan(unsigned long lFreq) {
      _CONSOLE_DEBUG(F("I2C: start scan with freq = %d kHz..."), lFreq/1000);
      
      int  nError = -1;
      bool bFound = false;
      
      m_bError = false;
      m_bChanged = false;
      m_bOnline = true;
      
      for (int i=1; i<128; i++) {
#ifdef ARDUINO
         Wire.setClock(lFreq);
         Wire.beginTransmission(i);
         nError = Wire.endTransmission();
#endif
         CxI2CDevice* pDev = findDevice(i);
         
         if (nError == 0) {
            bFound = true;
            
            if (pDev) {
               m_bChanged = true;
            } else {
               // new device
               pDev = new CxI2CDevice(i, this);
               m_mapDevices[i] = pDev;
            }
            
            if (pDev) {
               _CONSOLE_DEBUG(F("I2C: found Device at 0x%02X (%s) at freq %d kHz"), i, pDev->getTypeSz(), lFreq/1000);
               if (pDev->getType() == CxI2CDevice::EI2CDeviceType::bme) {
                  m_bBme = true;
               }
               if (pDev->getType() == CxI2CDevice::EI2CDeviceType::oled) {
                  m_bOled = true;
               }
               if (pDev) pDev->setError(false);
            }
         } else if (nError == 4) {
            m_bError = true;
            m_bChanged = true;
            if (i == 1) {
               _CONSOLE_DEBUG(F("I2C: ### general bus error"));
               m_bOnline = false;
               break;
            } else {
               _CONSOLE_DEBUG(F("I2C: ### error 4 at address %02X"), i);
               if (pDev) pDev->setError(true);
            }
         } else if (pDev) {
            m_bError = true;
            m_bChanged = true;
            _CONSOLE_DEBUG(F("I2C: lost Device at 0x%02X (error %d)"), i, nError);
            if (pDev) pDev->setError(true);
         }
      }
#ifdef ARDUINO
      Wire.setClock(getClock()); // reset to configured clock speed
#endif
   }
   
   void scan() {scan(100000);scan(400000);}
   
   tI2CDeviceMap& getDeviceMap() {return m_mapDevices;}
   
};

#endif /* CxI2CDevice_hpp */
