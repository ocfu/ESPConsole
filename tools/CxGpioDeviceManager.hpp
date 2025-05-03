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


class CxDevice {
   String _strName;
   String _strFriendlyName;
   String _strCmd;
   
   uint8_t _id = INVALID_UINT8;
   
   uint32_t _nDebounce = 100;
   

public:
   typedef std::function<void(CxDevice* dev, uint8_t id, const char* cmd)> cbFunc;
   
   void addCallback(cbFunc fp) {if(fp)__cbVec.push_back(fp);}
   
protected:
   bool __bPersistent = true;
   
   // callback vector
   std::vector<cbFunc> __cbVec;
   
   void callCb(uint8_t id, const char* cmd = nullptr) {
      for (auto& cb : __cbVec) {
         if (cb != nullptr) {
            cb(this, id, cmd ? cmd : getCmd());
         }
      }
   }
   
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();  /// Reference to the console instance

   /// Register the device with the manager
   void registerDevice();
   
   /// Unregister the device from the manager
   void unregisterDevice();
   
public:
   CxDevice(uint8_t id, const char* name, const char* cmd = "") : _id(id), _strName(name), _strCmd(cmd) {
      registerDevice();
   }
   
   virtual ~CxDevice() {
      unregisterDevice();
      end();
   }
   
   uint8_t getId() {return _id;}
   void setId(uint8_t id) {_id = id;}
   
   void setDebounce(uint32_t set) {_nDebounce = set;}
   uint32_t getDebounce() {return _nDebounce;}
   
   virtual void begin() {}
   virtual void loop(bool bDegraded = false) {}
   virtual void end() {}
   
   virtual const char* getTypeSz() = 0;
   
   void setCmd(String strCmd) {_strCmd = strCmd;}
   const char* getCmd() {return _strCmd.c_str();}
   
   void setFriendlyName(const char* name) {if (name) _strFriendlyName = name;}
   const char* getFriendlyName() {
      if (_strFriendlyName.length()) {
         return _strFriendlyName.c_str();
      } else {
         return getName();
      }
   }
   void setName(const char* name) {
      if (name) {
         _strName = __console.makeNameIdStr(name);
      }
   }
   const char* getName() {return _strName.c_str();}
   
   virtual const std::vector<String> getHeadLine(bool bDefault = true) = 0;
   virtual const std::vector<uint8_t> getWidths(bool bDefault = true) = 0;
   virtual const std::vector<String> getData(bool bDefault = true) = 0;
   
   virtual void set(int16_t set) = 0;
   virtual int16_t get() = 0;

};


class CxGPIODevice : public CxDevice, public CxGPIO {
   
public:
   CxGPIODevice(uint8_t pin, uint8_t mode = INVALID_MODE, bool inverted = false, const char* cmd = "") : CxDevice(pin, "", cmd), CxGPIO(pin, mode, inverted) {
      registerDevice();
   }
   
   virtual ~CxGPIODevice() {
      unregisterDevice();
      end();
   }

   virtual void set(int16_t set) override {CxGPIO::set(set);}
   virtual int16_t get() override {return CxGPIO::get();}
   
   virtual void begin() override {}
   virtual void loop(bool bDegraded = false) override {}
   virtual void end() override {}
   
   virtual const char* getTypeSz() override = 0;
   
   virtual const std::vector<String> getHeadLine(bool bDefault = true) override {
      return {F("Id"), F("Name"), F("Type"), F("GPIO"), F("Mode"), F("Inv"), F("State"), F("Cmd")};
   };
   
   virtual const std::vector<uint8_t> getWidths(bool bDefault = true) override {
      return {3, 11, 10, 4, 10, 3, 5, 20};
   };
   
   virtual const std::vector<String> getData(bool bDefault = true) override {
      return {String(getId()), getName(), getTypeSz(), String(getPin()).c_str(), getPinModeSz(), isInverted() ? "yes" : "no", getDigitalState() ? "on" : "off", getCmd()};
   };

};

class CxGPIOVirtual : public CxGPIODevice {
   
public:
   enum EVirtualEvent {evon, evoff};
   
private:
   
   static void _Action(CxDevice* dev, uint8_t id, const char* cmd) {
      if (!cmd  || !*cmd) return;
      
      String strCmd;
      strCmd.reserve((uint32_t)(strlen(cmd) + 10)); // preserve some space for the command and additional parameters.
      strCmd = cmd;
      
      if (id == CxGPIOVirtual::EVirtualEvent::evon) {
         strCmd.replace("$STATE", "ON");
         ESPConsole.processCmd(strCmd.c_str());
      } else if (id == CxGPIOVirtual::EVirtualEvent::evoff) {
         strCmd.replace("$STATE", "OFF");
         ESPConsole.processCmd(strCmd.c_str());
      }
   }
   
public:
   CxGPIOVirtual(uint8_t nPin = -1, const char* name = "", bool bInverted = false, const char* cmd = "", cbFunc fp = nullptr) : CxGPIODevice(nPin, INPUT, bInverted, cmd) { addCallback(fp); addCallback(_Action);setName(name);;}
   
   virtual ~CxGPIOVirtual() {end();}
   
   bool isOn() {return __gpioTracker.getDigitalState(getPin());}
   
   virtual void set(int16_t set) override {CxGPIODevice::set(set);callCb(set ? EVirtualEvent::evon : EVirtualEvent::evoff);}
   virtual int16_t get() override {return CxGPIODevice::get();}
      
   void on() {
      set(HIGH);
   }
   
   void off() {
      set(LOW);
   }

   virtual const char* getTypeSz() override {return "virtual";}
   
   virtual void loop(bool bDegraded = false) override {  // degraded: e.g. in Wifi in AP mode or esp in setup process. If degraded, then no callback and no relay action
   }
};



class CxGPIODeviceManagerManager {
private:

   /// gpio tracker instance
   CxGPIOTracker& _gpioTracker = CxGPIOTracker::getInstance();
   
   /// Map to store Devices with their unique IDs
   std::map<uint8_t, CxDevice*> _mapDevices;
   
   /// Private constructor to enforce singleton pattern
   CxGPIODeviceManagerManager() = default;
   /// Default destructor
   ~CxGPIODeviceManagerManager() = default;
   
protected:
   /// Reference to the console instance
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();


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
   
   /// add Device
   void addDevice(CxDevice* pDevice) {
      if (pDevice) {
         if (pDevice && pDevice->getId() != INVALID_UINT8) {
            /// Add the sensor to the map
            _mapDevices[pDevice->getId()] = pDevice;
         }
      }
   }

   void removeDevice(const char* name) {
      if (name) {
         for (auto& it : _mapDevices) {
            if (strcmp(it.second->getName(), name) == 0) {
               _mapDevices.erase(it.first);
               break;
            }
         }
      }
   }

   /// Get a Device by its name
   CxDevice* getDevice(const char* name, const char* type = nullptr) {
      if (name) {
         for (auto& it : _mapDevices) {
            if (strcmp(it.second->getName(), name) == 0 && (!type || strcmp(it.second->getTypeSz(), type) == 0)) {
               return it.second;
            }
         }
      }
      return nullptr;
   }
   
   /// get a device by its pin
   CxDevice* getDeviceByPin(uint8_t pin) {
      for (auto& it : _mapDevices) {
         if (it.second->getId() == pin) {
            return it.second;
         }
      }
      
      // double check with gpio tracker
      if (_gpioTracker.hasPin(pin)) {
         return getDevice(_gpioTracker.getName(pin));
      }
      
      return nullptr;
   }
   
   /// get a device by its type.
   /// The first device found will be returned.
   CxDevice* getDeviceByType(const char* type) {
      if (type) {
         for (auto& it : _mapDevices) {
            if (strcmp(it.second->getTypeSz(), type) == 0) {
               return it.second;
            }
         }
      }
      return nullptr;
   };

   /// get a device by its type.
   /// The first device found will be returned.
   CxDevice* getDeviceByName(const char* name) {
      if (name) {
         for (auto& it : _mapDevices) {
            if (strcmp(it.second->getName(), name) == 0) {
               return it.second;
            }
         }
      }
      return nullptr;
   };

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
      bool bDefault = (strType.length() == 0);
      
      CxTablePrinter table(*__console.getStream());
      
      /// iterate over all sensors and print formated sensor information
      for (const auto& [nId, pDevice] : _mapDevices) {
         if (strType.length() > 0 && strType != pDevice->getTypeSz()) {
            continue;
         }
         
         if (n++ == 0) {
            table.printHeader(pDevice->getHeadLine(bDefault), pDevice->getWidths(bDefault));
         }
         table.printRow(pDevice->getData(bDefault));
      }
   }
};

void CxDevice::registerDevice() {
   CxGPIODeviceManagerManager::getInstance().addDevice(this);
   
}

void CxDevice::unregisterDevice() {
   CxGPIODeviceManagerManager::getInstance().removeDevice(getName());
}

#endif /* CxGPIODeviceManager_hpp */
