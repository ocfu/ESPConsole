//
//  CxGPIODeviceManager.hpp
//  test
//
//  Created by ocfu on 31.07.22.
//

#ifndef CxGPIODeviceManager_hpp
#define CxGPIODeviceManager_hpp

#include "CxGpioTracker.hpp"
#include "CxTimer.hpp"
#include "CxConfigParser.hpp"


class CxGPIODevice : public CxGPIO {

   String _strName;
   String _strCmd;
   
   uint8_t _id = 0;
  
   /// Register the device with the manager
   void registerGPIODevice();
   
   /// Unregister the device from the manager
   void unregisterGPIODevice();
   
public:
   typedef std::function<void(CxGPIODevice* dev, uint8_t id, const char* cmd)> cbFunc;
   
   void addCallback(cbFunc fp) {if(fp)__cbVec.push_back(fp);}
   
protected:
   // callback vector
   std::vector<cbFunc> __cbVec;
   
   void callCb(uint8_t id, const char* cmd = nullptr) {
      for (auto& cb : __cbVec) {
         if (cb != nullptr) {
            cb(this, id, cmd ? cmd : getCmd());
         }
      }
   }

   
public:
   CxGPIODevice(uint8_t pin, uint8_t mode = INVALID_MODE, bool inverted = false, const char* cmd = "") : CxGPIO(pin, mode, inverted), _strCmd(cmd) {
      registerGPIODevice();
   }
   CxGPIODevice() : CxGPIO(-1) {}
   //CxGPIODevice(uint8_t nPin, isr_t p, uint8_t mode = 0x1) : CxGPIO(nPin, p, mode) {}
   
   virtual ~CxGPIODevice() {
      unregisterGPIODevice();
      end();
   }

   uint8_t getId() {return _id;}
   void setId(uint8_t id) {_id = id;}
   
   virtual void begin() {}
   virtual void loop(bool bDegraded = false) {}
   virtual void end() {}
   
   virtual const char* getTypeSz() = 0;
   
   void setCmd(String strCmd) {_strCmd = strCmd;}
   const char* getCmd() {return _strCmd.c_str();}
   
   void setName(const char* name) {_strName = name;}
   const char* getName() {return _strName.c_str();}
   
   void printDefaultHeadLine() {
      __console.printf(F(ESC_ATTR_BOLD "Name         | Type       | GPIO | Mode       | Inv | State " ESC_ATTR_RESET));
   }
   
   virtual void printHeadLine(bool bGeneral = true) {
      __console.printf(F(ESC_ATTR_BOLD "| PWM | Analog | Command " ESC_ATTR_RESET ));
   }
   
   void printDefaultData() {
      __console.printf(F(ESC_ATTR_BOLD "%-11s " ESC_ATTR_RESET " | %-10s | %02d   | %-10s | %-3s |"), getName(), getTypeSz(), getPin(), getPinModeString().c_str(), isInverted() ? "yes" : "no");
      
      if (isInverted()) {
         __console.printf("!");
      } else {
         __console.printf(" ");
      }
      __console.printf(F("%-5s "), getDigitalState() ? "on" : "off");
   }
   
   virtual void printData(bool bGeneral = true) {
      __console.printf(F("| %-3s | "), isPWM() ? "yes" : "no");
      if (isAnalog()) {
         __console.printf(F(" %6d | "), getAnalogValue());
      } else {
         __console.printf(F("       | "));
      }
      __console.printf(F("%s "), getCmd());
   }

};


class CxGPIODeviceManagerManager {
private:
   /// Reference to the console instance
   CxESPConsoleMaster& _console = CxESPConsoleMaster::getInstance();
   
   /// gpio tracker instance
   CxGPIOTracker& _gpioTracker = CxGPIOTracker::getInstance();
   
   /// Map to store Devices with their unique IDs
   std::map<uint8_t, CxGPIODevice*> _mapDevices;
   
   /// Private constructor to enforce singleton pattern
   CxGPIODeviceManagerManager() = default;
   /// Default destructor
   ~CxGPIODeviceManagerManager() = default;

public:
   /// Access the singleton instance of CxGPIODeviceManagerManager
   static CxGPIODeviceManagerManager& getInstance() {
      static CxGPIODeviceManagerManager instance; // Constructed on first access
      return instance;
   }
   
   /// Disable copying and assignment to enforce singleton pattern
   CxGPIODeviceManagerManager(const CxGPIODeviceManagerManager&) = delete;
   CxGPIODeviceManagerManager& operator=(const CxGPIODeviceManagerManager&) = delete;
   
   /// Get the number of registered devices
   uint8_t getDeviceCount() {return _mapDevices.size();}
   
   /// unique id
   uint8_t createId() {
      uint8_t id = 0;
      while (_mapDevices.find(id) != _mapDevices.end()) {
         id++;
      }
      return id;
   }
   
   /// add Device
   void addDevice(CxGPIODevice* pDevice) {
      if (pDevice) {
         uint8_t nId = createId();
         /// Set the unique ID for the sensor
         pDevice->setId(nId);
         /// Add the sensor to the map
         _mapDevices[nId] = pDevice;
      }
   }
   
   void removeDevice(uint8_t id) {
      if (_mapDevices.find(id) != _mapDevices.end()) {
         _mapDevices.erase(id);
      }
   }
   
   void removeDevice(const char* name) {
      for (auto& it : _mapDevices) {
         if (strcmp(it.second->getName(), name) == 0) {
            _mapDevices.erase(it.first);
            break;
         }
      }
   }
   
   /// Get a Device by its unique ID
   CxGPIODevice* getDevice(uint8_t id) {
      if (_mapDevices.find(id) != _mapDevices.end()) {
         return _mapDevices[id];
      }
      return nullptr;
   }
   
   /// Get a Device by its name
   CxGPIODevice* getDevice(const char* name) {
      for (auto& it : _mapDevices) {
         if (strcmp(it.second->getName(), name) == 0) {
            return it.second;
         }
      }
      return nullptr;
   }
   
   /// get a device by its pin
   CxGPIODevice* getDeviceByPin(uint8_t pin) {
      for (auto& it : _mapDevices) {
         if (it.second->getPin() == pin) {
            return it.second;
         }
      }
      
      // double check with gpio tracker
      if (_gpioTracker.hasPin(pin)) {
         return getDevice(_gpioTracker.getName(pin));
      }
      
      return nullptr;
   }
   
   /// pin in use?
   bool isPinInUse(uint8_t pin) {
      for (auto& it : _mapDevices) {
         if (it.second->getPin() == pin) {
            return true;
         }
      }
      
      // double check with gpio tracker
      if (_gpioTracker.hasPin(pin)) {
         return true;
      }
      
      return false;
   }
   
   /// Loop through all Devices and update their state
   void loop(bool bDegraded = false) {
      for (auto& it : _mapDevices) {
         it.second->loop(bDegraded);
      }
   }
   
   /**
    * @brief Print a list of all devices to the console.
    */
   void printList(String strType = "") {
      /// print table header (ID, name, type,...)

      uint8_t n = 0;
      bool bGeneral = (strType.length() == 0);
      
      /// iterate over all sensors and print formated sensor information
      for (const auto& [nId, pDevice] : _mapDevices) {
         if (strType.length() > 0 && strType != pDevice->getTypeSz()) {
            continue;
         }
         
         if (n++ == 0) {
            pDevice->printDefaultHeadLine();
            pDevice->printHeadLine(bGeneral);
            _console.println();
         }

         pDevice->printDefaultData();
         pDevice->printData(bGeneral);
         _console.println();
      }
   }
};

void CxGPIODevice::registerGPIODevice() {
   CxGPIODeviceManagerManager::getInstance().addDevice(this);
   
}

void CxGPIODevice::unregisterGPIODevice() {
   CxGPIODeviceManagerManager::getInstance().removeDevice(getId());
}

#endif /* CxGPIODeviceManager_hpp */
