//
//  CxMqttHAManager.hpp
//
//
//  Summary:
//  This file defines a set of classes for managing MQTT-based Home Assistant (HA) entities.
//  It provides a framework for creating and managing HA-compatible devices, sensors, buttons,
//  switches, and other entities. The classes handle MQTT topic generation, JSON configuration
//  for HA discovery, and state/attribute publishing.
//
//  Key Features:
//  - Supports HA discovery for various entity types (sensor, button, light, switch, etc.).
//  - Automatically generates MQTT topics and JSON payloads for HA integration.
//  - Manages entity availability (online/offline) and state updates.
//  - Provides a singleton-based device manager for grouping entities.
//
// Relationship Between CxMqttHABase and CxMqttHADevice
//
// Feature              CxMqttHABase (Entity Base)       CxMqttHADevice (Device Manager)
// ----------------------------------------------------------------------------------------------
// Role                 Base class for HA entities       Manages HA device & entity collection
// MQTT Topic Handling  Defines base topic path          Assigns topics to entities
// Entity Types         Defines multiple HA types        Groups and manages multiple entities
// Discovery Handling   Implements setDiscoveryTopic()   Calls regDiscovery() on all entities
// Singleton            No                               Yes (manages all entities centrally)
// Debugging            Uses CxESPConsole                Logs HA discovery and registration
// Thread Safety Issues Missing mutex for state updates  No mutex for _vecItems management
//
// How They Work Together
// 1. CxMqttHADevice creates and manages multiple CxMqttHABase entities.
// 2. CxMqttHADevice::addItem() adds entities (e.g., sensors, buttons) to _vecItems.
// 3. CxMqttHADevice::regItems() registers/deregisters all entities with HA.
// 4. CxMqttHADevice::publishAvailabilityAllItems() updates availability for all entities.
//
// Suggested Improvements
//
// To improve thread safety and memory management, consider these enhancements:
//
// 1) Add Mutex Protection for Thread Safety
// Use std::mutex (or portMUX_TYPE on ESP32) for:
//
// _vecItems modifications in addItem(), regItems(), publishAvailabilityAllItems().
// Entity state updates in CxMqttHABase.
//
//
//  References:
//  - Home Assistant MQTT Discovery: https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
//  - Sensor Device Classes: https://www.home-assistant.io/integrations/sensor#device-class
//  - State Classes: https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes
//
//
//


#ifndef CxMqttHA_hpp
#define CxMqttHA_hpp

#include "CxMqttManager.hpp"
#include "espmath.h"
#include "ArduinoJson.h"

// Forward declaration for CxMqttHADevice
class CxMqttHADevice;

/**
 * @class CxMqttHABase
 * @brief A base class for managing Home Assistant (HA) entities using CxMqttTopicBase.
 *
 * This class provides foundational functionality for HA entity management, including:
 * - Entity identification and topic generation
 * - Support for multiple HA entity types (sensor, switch, button, etc.)
 * - Debugging integration through CxESPConsole
 *
 * ## Key Features:
 * - Stores sanitized entity names and unique identifiers.
 * - Defines a structured enumeration for different HA entity types.
 * - Utilizes a debug console instance for logging and diagnostics.
 *
 * ## Known Issues:
 * - Missing mutex for thread safety. Since MQTT communication and entity management may involve multiple threads, lack of mutexes could cause race conditions when updating state or publishing messages.
 */
class CxMqttHABase : public CxMqttTopicBase {
private:
   String _strName;                ///< Sanitized entity name (lowercase/underscores)
   String _strId;                  ///< Unique identifier (chipID + entity name)
   String _strTopicBase;           ///< Base MQTT topic path
   
protected:
   CxESPConsoleMaster& __console = CxESPConsoleMaster::getInstance();

public:
   /**
    * @enum e_type
    * @brief Supported Home Assistant entity types
    */
   enum class e_type  {
      none = 0,      ///< Unspecified type
      HAdevice,      ///< Parent device container
      HAsensor,      ///< Sensor entity
      HAbutton,      ///< Button entity
      HAlight,       ///< Light entity
      HAtext,        ///< Text input entity
      HAswitch,      ///< Switch entity
      HAbinary,      ///< Binary sensor entity
      HAnumber,      ///< Number input entity
#ifndef MINIMAL_COMMAND_SET
      HAsiren,       ///< Siren entity
      HAalarmpanel,  ///< Alarm panel entity
      HAnotify,      ///< Notification entity
      HAevent,       ///< Event entity
#endif
      HAselect       ///< Select entity
   };
   
   /**
    * @enum e_cat
    * @brief Entity organization categories
    */
   enum class e_cat   {
      none = 0,      ///< Default category
      config,        ///< Configuration entity
      diagnostic     ///< Diagnostic entity
   };
   
   /**
    * @enum e_state
    * @brief Sensor state classifications
    */
   enum class e_state {
      none = 0,      ///< No classification
      measurement,   ///< Current measurement value
      total,         ///< Total accumulated value
      total_inc      ///< Monotonically increasing total
   };
   
   /**
    * @brief Get entity type as string
    * @return const char* HA-compatible type identifier
    */
   const char* getTypeSz() const {
      switch(__eType) {
         case e_type::none: return "none";
         case e_type::HAdevice: return "device";
         case e_type::HAsensor: return "sensor";
         case e_type::HAbutton: return "button";
         case e_type::HAlight: return "light";
         case e_type::HAtext: return "text";
         case e_type::HAswitch: return "switch";
         case e_type::HAbinary: return "binary_sensor";
         case e_type::HAnumber: return "number";
#ifndef MINIMAL_COMMAND_SET
         case e_type::HAsiren: return "siren";
         case e_type::HAalarmpanel: return "alarm_control_panel";
         case e_type::HAnotify: return "notify";
         case e_type::HAevent: return "event";
#endif
         case e_type::HAselect: return "select";
         default: return "";
      }
   }
   
   /**
    * @brief Get category as string
    * @return const char* HA-compatible category identifier
    */
   const char* getCatSz() const {
      switch(__eCat) {
         case e_cat::none: return "none";
         case e_cat::config: return "config";
         case e_cat::diagnostic: return "diagnostic";
         default: return "";
      }
   }
   
   /**
    * @brief Get state class as string
    * @return const char* HA-compatible state class identifier
    */
   const char* getStateClassSz() const {
      switch(__eStateClass) {
         case e_state::none: return "none";
         case e_state::measurement: return "measurement";
         case e_state::total: return "total";
         case e_state::total_inc: return "total_increasing";
         default: return "";
      }
   }

private:
   // Configuration
   String _strTopicHADiscovery;    ///< Discovery topic path
   String _strTopicHAAction;       ///< Action topic path
   CxMqttTopic _mqttCmdTopic;      ///< Command topic handler
   const char* _szFriendlyName;    ///< User-friendly display name
   const char* _szTopicBaseHADiscovery = "/homeassistant"; ///< HA discovery prefix
   bool _bEnabledByDefault;        ///< Entity enabled by default in HA
   
   // State
   bool _bAvailable;               ///< Online/offline status
   bool _bState;                   ///< Current state (ON/OFF)
   bool _bRetainedCmd;             ///< Retain command messages
   
protected:
   e_type      __eType;            ///< Entity type
   e_cat       __eCat;             ///< Entity category
   e_state     __eStateClass;      ///< State classification
   bool        __bCmd;             ///< Supports commands
   CxMqttHADevice* __pDev;         ///< Parent device reference
   const char* __szAction;         ///< Automation action type
   
   /** @brief Special constructor for derived classes */
   explicit CxMqttHABase(bool) : CxMqttTopicBase() {}

public:
   /** @brief Default constructor (registers with device manager) */
   CxMqttHABase() : CxMqttTopicBase() {registerDevice<CxMqttHADevice>();}
   
   /**
    * @brief Parameterized constructor
    * @param fn Friendly name
    * @param name Sanitized identifier
    * @param topicbase Base MQTT topic path
    * @param cb MQTT callback function
    * @param topic Specific MQTT topic
    * @param retain Retain messages flag
    */
   CxMqttHABase(const char* fn, const char* name, const char* topicbase = nullptr,
                CxMqttManager::tCallback cb = nullptr, const char* topic = nullptr,
                bool retain = false)
   : CxMqttTopicBase(nullptr, cb, retain), _szFriendlyName(fn),
   __eType(e_type::none), __eCat(e_cat::none), __eStateClass(e_state::none),
   _bAvailable(false), __bCmd(false), __pDev(nullptr), __szAction(nullptr),
   _mqttCmdTopic(nullptr, cb), _bRetainedCmd(false), _bEnabledByDefault(true) {
      setName(name);
      registerDevice<CxMqttHADevice>();
      setTopicBase(topicbase);
      setTopic(topic);
      setStrId();
   }
   
   /**
    * @brief Destructor
    */
   ~CxMqttHABase() {
      unregisterDevice<CxMqttHADevice>();
   }
   
   /**
    * @brief Generate unique entity ID
    */
   void setStrId() {
      _strId = "cx";
      _strId += String(getChipId(), 16);
      _strId += "_";
      _strId += getName();
   }
   
   bool hasCmdIF() {return __bCmd;}
   
   void setCmdCb(CxMqttManager::tCallback cb) {_mqttCmdTopic.setCb(cb);}
   
   /**
    * @brief Sanitize and set entity name
    * @param sz Raw name input
    */
   void setName(const char* sz) {
      _strName = __console.makeNameIdStr(sz);
   }
   
   /**
    * @brief Get sanitized entity name
    * @return const char* Sanitized name
    */
   const char* getName() const {return _strName.c_str();}
   
   /**
    * @brief Set entity category
    * @param cat Category enum value
    */
   void setCat(e_cat cat) { __eCat = cat;}
   
   /**
    * @brief Get current category
    * @return e_cat Current category
    */
   e_cat getCat() const {return __eCat;}

#ifndef MINIMAL_COMMAND_SET
   /**
    * @brief Mark entity as configuration category
    * @details Sets entity category to config and enables command retention
    */
   void asConfig() {
      setCat(e_cat::config);
      setRetainedCmd(true);
   }
   
   /**
    * @brief Mark entity as diagnostic category
    */
   void asDiagnostic() { setCat(e_cat::diagnostic); }
   
   /**
    * @brief Reset entity to default category (none)
    */
   void asDefault() { setCat(e_cat::none); }
#endif

   /// From CxMqttHABase derived classes need to register to CxMqttHADevice, which is also a derived class from CxMqttHABase and not known here, so we are using the template "trick" to solve this and to avoid the need of a cpp file.
   /**
    * @brief Register entity with device manager
    * @tparam T Device manager class (must implement singleton pattern)
    */
   template <typename T>
   void registerDevice() {
      T::getInstance().addItem(this);  // Calls the singleton instance of T to register the new ha object
   }

   // unregister from device
   /**
    * @brief Unregister entity with device manager
    * @tparam T Device manager class (must implement singleton pattern)
    */
   template <typename T>
   void unregisterDevice() {
      T::getInstance().delItem(this);  // Calls the singleton instance of T to unregister the ha object
   }
   
   /**
    * @brief Get base topic from associated device
    * @tparam T Device manager class
    * @return const char* Base MQTT topic path
    */
   template <typename T>
   const char* getTopicBaseFromDevice() const {
      return T::getInstance().getTopicBase();
   }
   
   /**
    * @brief Get name of associated device
    * @tparam T Device manager class
    * @return const char* Device name
    */
   template <typename T>
   const char* getDeviceName() const {
      return T::getInstance().getName();
   }
   
   /**
    * @brief Set friendly display name
    * @param fn Name to display in Home Assistant UI
    */
   void setFriendlyName(const char* fn) {_szFriendlyName = fn;}
   const char* getDeviceName() const {return getDeviceName<CxMqttHADevice>();}
   
   const char* getId() const {return _strId.c_str();}
   const char* getTopicBase() const {return _strTopicBase.c_str();}
   void setTopicBase(const char* topicbase) {
      if (topicbase) _strTopicBase = topicbase;
   }

   const char* getFriendlyName() const {return _szFriendlyName;}
   const char* getTopicHADiscovery() const {return _strTopicHADiscovery.c_str();}
   const char* getTopicHAAction() const {return _strTopicHAAction.c_str();}
   
   /**
    * @brief Check if entity is enabled by default in HA
    * @return bool True if enabled by default
    */
   bool isEnabledByDefault() const {return _bEnabledByDefault;}
   
   /**
    * @brief Set default enable state
    * @param set True to enable by default
    */
   void setEnabledByDefault(bool set) {_bEnabledByDefault = set;}
   
   /**
    * @brief Check entity availability status
    * @return bool True if entity is available (online)
    */
   bool isAvailable() const {return _bAvailable;}
   void setAvailable(bool set) {_bAvailable = set;}
   
   bool getState() const {return _bState;}
   
   /**
    * @brief Set command retention flag
    * @param set True to retain command messages
    */
   void setRetainedCmd(bool set) {_bRetainedCmd = set;}
   bool isRetainedCmd() const {return _bRetainedCmd;}

   bool isAction() const {return (__szAction != nullptr);}
   const char* getDeviceId() const {
      if (__pDev) {
         return ((CxMqttHABase*)__pDev)->getId();
      } else {
         return "other";
      }
   }
   
   /**
    * @brief Build discovery topic path
    * @details Format: homeassistant/<type>/<device_id>/<entity_id>/config
    */
   void setDiscoveryTopic() {
      // FIXME: if the device ID or name is too long, this could exceed MQTT topic length limits. Maybe adding length checks or truncation would help.
      
      _strTopicHADiscovery = _szTopicBaseHADiscovery;
      _strTopicHADiscovery += "/";
      _strTopicHADiscovery += getTypeSz();
      _strTopicHADiscovery += "/";
      _strTopicHADiscovery += getDeviceId();
      _strTopicHADiscovery += "/";
      _strTopicHADiscovery += getName();
      _strTopicHADiscovery += "/config";

      _CONSOLE_DEBUG("MQTTHA: topicDiscovery=%s", _strTopicHADiscovery.c_str());
      if (isAction()) {
         _CONSOLE_DEBUG("MQTTHA: topicAction=%s", _strTopicHAAction.c_str());
      }
   }
   
   /**
    * @brief Build action topic path
    * @details Format: homeassistant/device_automation/<entity_id>/<action>
    */
   void setActionTopic () {
      // !! note: device_automation/<ID> is mandatory (no further sub-structure e.g. to separate devices)
      _strTopicHAAction = _szTopicBaseHADiscovery;
      _strTopicHAAction += "/device_automation/";
      _strTopicHAAction += getId();
      _strTopicHAAction += "/";
      _strTopicHAAction += __szAction;
      
      _CONSOLE_DEBUG("MQTTHA: topicAction=%s", _strTopicHAAction.c_str());
   }
   
   void setDev(CxMqttHADevice* pDev) {__pDev = pDev;}
   
   virtual void addJsonConfig(JsonDocument& doc) const {};
   virtual void addJsonAction(JsonDocument& doc) const {};
   
   /**
    * @brief Add base configuration to discovery payload
    * @param doc JSON document to populate
    */
   void addJsonConfigBase(JsonDocument &doc) {
      static char szTopicBase[126];
      snprintf(szTopicBase, sizeof(szTopicBase), "%s/%s", getRootPath(), getTopicBase());
      doc["~"] = szTopicBase;
      doc[F("name")] = getFriendlyName();
      doc[F("uniq_id")] = getId();
      doc[F("obj_id")] = getId(); // pre-defines the entity id in HA
      doc[F("stat_t")] = F("~/state");
      doc[F("val_tpl")] = F("{{ value_json.value }}");
      
      // two conditions for the availability: device and sensor must be online
      if (__pDev) {
         JsonArray array = doc.createNestedArray(F("avty"));
         JsonObject objDev = array.createNestedObject();
         snprintf(szTopicBase, sizeof(szTopicBase), "%s", getRootPath());
         objDev[F("t")] = szTopicBase;
         if (__eCat != e_cat::diagnostic) {
            JsonObject objSen = array.createNestedObject();
            objSen[F("t")] = "~";
         }
         doc[F("avty_mode")] = F("all");
      } else {
         doc[F("avty")]["t"] = "~"; // only one condition
      }
      
      if (__eStateClass != e_state::none) {
         doc[F("stat_c")] = getStateClassSz();
      }
      
      if (__bCmd) {
         doc[F("cmd_t")] = F("~/cmd");
         doc[F("retain")] = isRetainedCmd();
         doc[F("qos")] = 1;
         doc[F("stat_val_tpl")] = F("{{ value_json.state }}");
         doc[F("en")] = isEnabledByDefault();
      }
      doc[F("json_attr_t")] = F("~/attributes");
      switch (__eCat) {
         case e_cat::diagnostic:
            doc[F("ent_cat")] = F("diagnostic"); // ! null value does not work in ha for entity_category
            break;
         case e_cat::config:
            doc[F("ent_cat")] = F("config"); // ! null value does not work in ha for entity_category
            break;
         default:
            break;
      }

   }
   
   /**
    * @brief Add base action configuration
    * @param doc JSON document to populate
    */
   void addJsonActionBase(JsonDocument &doc) const {
      doc["~"] = getTopicBase();
      doc[F("atype")] = F("trigger");
      doc[F("type")] = F("action");
      doc[F("topic")] = F("~/state");
      doc[F("val_tpl")] = F("{{ value_json.value }}");
      

   }
   
   void getConfigPayload(String& str) {
      
      DynamicJsonDocument doc(1024);
      
      // add the base config elements
      addJsonConfigBase(doc);
      
      // add config elements from the individual class, which must have implemented this virtual function.
      addJsonConfig(doc);
      
      // add device config elements from the linked device
      if (__pDev) ((CxMqttHABase*)__pDev)->addJsonConfig(doc);
      
#ifdef ARDUINO
      serializeJsonPretty(doc, str);
#else
      char szJson[1024];
      serializeJsonPretty(doc, szJson, sizeof(szJson));
      str = szJson;
#endif
      
   }
   
   void getActionPayload(String& str) {
      
      DynamicJsonDocument doc(1024);

      // add the base config elements
      addJsonActionBase(doc);
      
      // add config elements from the individual class, which must have implemented this virtual function.
      addJsonAction(doc);
            
      // add device config elements from the linked device
      if (__pDev) ((CxMqttHABase*)__pDev)->addJsonConfig(doc);
      
#ifdef ARDUINO
      serializeJsonPretty(doc, str);
#else
      char szJson[1024];
      serializeJsonPretty(doc, szJson, sizeof(szJson));
      str = szJson;
#endif

   }
   
   void regDiscovery(bool bEnable) {
      String strPayload;
      strPayload.reserve(1024);

      if (bEnable) {
         getConfigPayload(strPayload);
         
         publish(getTopicHADiscovery(), strPayload.c_str(), true); // set retain flag
         
         if (__bCmd) {
            subscribeCmd();
         }
         
         if (hasTopic()) {
            subscribe();
         }
         
         if (isAction()) {
            getActionPayload(strPayload);
            publish(getTopicHAAction(), strPayload.c_str(), true);
         }
         
         publishAvailability();
         
      } else {
         // unregister from HA discovery -> HA will remove the entity and device, if it is the last one
         // retain must be set to true, otherwise the mqtt broker will keep the message retained http://www.steves-internet-guide.com/mqtt-retained-messages-example/
         publish(getTopicHADiscovery(), "", true);
         if (__bCmd) {
            unsubscribeCmd();
         } else if (hasTopic()) {
            unsubscribe();
         }
         if (isAction()) {publish(getTopicHAAction(), "", true);}
      }
   }
   
   //--------------------------------------------------
   // MQTT Operations
   //--------------------------------------------------

   void publishState(double fValue, uint8_t prec = 2) {
      DynamicJsonDocument doc(256);
      doc["value"] = roundToPrecision(fValue, prec);
      publishState(doc);
   }
      
   void publishState(const char* szValue) {
      DynamicJsonDocument doc(256);
      doc["value"] = szValue;
      publishState(doc);
   }
   
   /**
    * @brief Publish entity state (boolean)
    * @param bState New state value
    */
   void publishState(bool bState) {
      DynamicJsonDocument doc(256);
      _bState = bState;
      
      doc["value"] = _bState ? "ON" : "OFF";;
      publishState(doc);
   }

   /**
    * @brief Publish entity state from JSON document
    * @param doc JSON document containing state data
    */
   void publishState(JsonDocument& doc) {
      String strTopic;
      String strPayload;
      
      doc["state"] = getState() ? "ON" : "OFF";
      serializeJson(doc, strPayload);
      strTopic = getTopicBase();
      strTopic += "/state";
            
      if (publish(strTopic.c_str(), strPayload.c_str(), isRetained()) && !isAvailable()) {
         // every published state should make the related entity available automatically.
         publishAvailability(true);
      }
   }

   /**
    * @brief Publish entity availability status
    * @param bAvailable New availability state
    */
   void publishAvailability(bool bAvailable) {
      if (_bAvailable != bAvailable) {
         _bAvailable = bAvailable;
         publishAvailability();
      }
   }
   
   /**
    * @brief Publish current availability status
    */
   void publishAvailability() {
      // topicbase is used for the availability topic
      _CONSOLE_DEBUG("MQTTHA: topic %s availability=%s", getTopicBase(), _bAvailable ? "online":"offline");

      publish(getTopicBase(), _bAvailable ? "online":"offline", true);
   }
   
   void publishAttributes(const char* szJsonAttr) {
      String strTopic;
      
      strTopic = getTopicBase();
      strTopic += "/attribute";
      
      if (publish(strTopic.c_str(), szJsonAttr, isRetained()) && !isAvailable()) {
         publishAvailability(true);
      };
   }
   
   void publishAttributes(JsonDocument &doc) {
      String strJsonAttributes;
      serializeJson(doc, strJsonAttributes);
      publishAttributes(strJsonAttributes.c_str());
   }
   
   /**
    * @brief Subscribe to command topic
    * @param bSubscribe True to subscribe, false to unsubscribe
    */
   void subscribeCmd(bool bSubscribe = true) {
      String strTopic;
      
      strTopic = getTopicBase();
      strTopic += "/cmd";
      
      _mqttCmdTopic.setTopic(strTopic.c_str());
      
      if (bSubscribe) {
         _mqttCmdTopic.subscribe();
      } else {
         // if (isRetainedCmd()) // no need as there is no harm if the cmd topic was actually not published with a retained flag.
         // Remember: HA (as the publisher of the cmd) is setting the retained flag, if 'retain' is set to true in the configuration for the entity.
         // The ha item is always the owner of a cmd topic and there will (normally) be no other subscriber to the cmd topic. The last message and its retained one can be removed. Note: Without set the retain flag to true, the broker would keep the last retained message.
         _CONSOLE_DEBUG("MQTTHA: removing the cmd topic %s and its retained one.", strTopic.c_str());
         publish(strTopic.c_str(), "", true);
         _mqttCmdTopic.unsubscribe();
      }
   }
   
   void unsubscribeCmd() {
      subscribeCmd(false);
   }

};


//-------------------------------------------------------------------------------------
/**
 * @class CxMqttHADevice
 * @brief A singleton class managing Home Assistant (HA) devices and their entities.
 *
 * This class acts as a central manager for HA-compatible devices, storing device metadata
 * and managing a collection of HA entities. It ensures proper MQTT topic organization
 * and handles discovery registration.
 *
 * ## Key Features:
 * - Implements the Singleton pattern to maintain a single device instance.
 * - Stores device metadata (manufacturer, model, software/hardware version, URL).
 * - Manages a collection of `CxMqttHABase` entities.
 * - Registers and deregisters all associated entities for HA discovery.
 * - Ensures proper topic base generation for managed entities.
 * - Supports JSON configuration for HA integration.
 * - Provides methods to publish availability states for all entities.
 *
 * ## Design Details:
 * - Prevents copy/move construction to enforce singleton behavior.
 * - Uses a private vector to track associated HA entities.
 * - Supports adding/removing entities dynamically.
 * - Offers debug logging for entity registration and availability updates.
 *
 * ## Known Issues:
 * - Missing Thread Safety
 *    - addItem(), regItems(), and _vecItems manipulation lack mutex protection.
 *    - Multiple threads updating entities could cause inconsistent MQTT states.
 *    - Implement Logging for Thread Safety Debugging
 */
class CxMqttHADevice : public CxMqttHABase {
private:
   String _strDevName;             ///< Device name
   const char* _szManufacturer;    ///< Manufacturer name
   const char* _szModel;           ///< Device model
   const char* _szSwVersion;       ///< Software version
   const char* _szHwVersion;       ///< Hardware version
   const char* _szUrl;             ///< Product URL
   typedef std::vector<CxMqttHABase*> _t_vecItems;
   _t_vecItems _vecItems;          ///< Managed entities
   
   ~CxMqttHADevice() {
      for (auto item : _vecItems) {
         delete item;  // Prevent memory leaks
      }
      _vecItems.clear();
   }
   
   CxMqttHADevice() : CxMqttHABase(true) {
      __pDev = this;
      __eType = e_type::HAdevice;
   }

protected:
   std::function<void()> _cbOnEnable = nullptr; ///< Command callback function
   
public:
   
   /** @brief Delete copy constructor and assignment operators to prevent copies. */
   CxMqttHADevice(const CxMqttHADevice&) = delete;
   CxMqttHADevice& operator=(const CxMqttHADevice&) = delete;
   
   /** @brief Delete move constructor and move assignment operator to prevent moves. */
   CxMqttHADevice(CxMqttHADevice&&) = delete;
   CxMqttHADevice& operator=(CxMqttHADevice&&) = delete;

   /**
    * @brief Retrieves the singleton instance of CxMqttHADevice.
    *
    * If an instance does not exist, it creates one.
    *
    * @return Reference to the singleton instance.
    */
   static CxMqttHADevice& getInstance() {
      static CxMqttHADevice _instance;
      return _instance;
   }
   
   void setCallbackOnEnable(std::function<void()> cb) {
      _cbOnEnable = cb;
   };
   
   void setManufacturer(const char* mf) {_szManufacturer = mf;}
   void setModel(const char* mdl) {_szModel = mdl;}
   void setSwVersion(const char* ver) {_szSwVersion = ver;}
   void setHwVersion(const char* ver) {_szHwVersion = ver;}
   void setUrl(const char* url) {_szUrl = url;}
      
   /**
    * @brief Add entity to device
    * @param item Entity to add
    * @param available Initial availability state
    */
   void addItem(CxMqttHABase* item, bool available = false) {
      if (item) {
         _CONSOLE_DEBUG("add item %s to HA", item->getFriendlyName());
         item->setAvailable(available);
         _vecItems.push_back(item);
      } else {
         _CONSOLE_DEBUG("item not valid to add to HA");
      }
   }

   /**
    * @brief Remove entity from device
    * @param item Entity to remove
    */
   void delItem(CxMqttHABase* item) {
      _t_vecItems::iterator it = _vecItems.begin();
      
      while(it != _vecItems.end())
      {
         if ((*it) == item) {
            _CONSOLE_DEBUG("delete item %s from HA", item->getFriendlyName());
            _vecItems.erase(it);
            //delete item;
            break;
         }
         it++;
      }
   }
   
   /**
    * @brief Register/deregister all entities
    * @param bEnable True to register, false to deregister
    */
   void regItems(bool bEnable = true) {
      _t_vecItems::iterator it = _vecItems.begin();
      
      _CONSOLE_DEBUG("%s %d items to HA", bEnable?"register":"unregister", _vecItems.size());
            
      while(it != _vecItems.end())
      {
         if ((*it) != this) {
            // the device defines the topic base by dedault
            char szTopicBase[126];
            snprintf(szTopicBase, sizeof(szTopicBase), "%s/%s", getTopicBase(), (*it)->getName());
            (*it)->setTopicBase(szTopicBase);
            (*it)->setDev(this);
            (*it)->setDiscoveryTopic();
            
            (*it)->regDiscovery(bEnable);
            
            if (bEnable) {
               if (_cbOnEnable) {
                  _cbOnEnable();
               }
            }
         }
         it++;
      }
   }
   
   /**
    * @brief Find entity by name
    * @param szName Entity name
    * @return Pointer to entity or nullptr if not found
    */
   CxMqttHABase* findItem(const char* szName) {
      _t_vecItems::iterator it = _vecItems.begin();
      
      while(it != _vecItems.end())
      {
         if (strcmp((*it)->getName(), szName) == 0) {
            return *it;
         }
         it++;
      }
      return nullptr;
   }

   
   void publishAvailabilityAllItems() {
      _t_vecItems::iterator it = _vecItems.begin();
      
      while(it != _vecItems.end())
      {
         (*it)->publishAvailability();
         it++;
      }
      publishAvailability(); // including me
   }
   
   void addJsonConfig(JsonDocument &doc) const {
      
      doc[F("dev")][F("name")] = getFriendlyName();
      doc[F("dev")][F("ids")] = getId();
      if(_szManufacturer) doc[F("dev")][F("mf")] = _szManufacturer;
      if(_szModel) doc[F("dev")][F("mdl")] = _szModel;
      if(_szSwVersion) doc[F("dev")][F("sw")] = _szSwVersion;
      if(_szHwVersion) doc[F("dev")][F("hw")] = _szHwVersion;
      if(_szUrl && strncmp(_szUrl, "http", 4) == 0) doc[F("dev")][F("cu")] = _szUrl;
      
   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
   void printList(Stream& stream) {
      CxTablePrinter table(stream);
      
      std::vector<String> vHeadLine = {F("Nr"),F("Name"),F("Friendly Name"),F("Type"), F("Available"),F("Retained"),F("Topic Base"), F("Has Cb"), F("/cmd")};
      std::vector<uint8_t> vWidths =  {  3,  20,  20,  10,   9,   8,  30, 6, 10};
      
      table.printHeader(vHeadLine, vWidths);
     
      _t_vecItems::iterator it = _vecItems.begin();
      
      uint8_t n = 1;
      
      while(it != _vecItems.end())
      {
         table.printRow({String(n).c_str(), (*it)->getName(), (*it)->getFriendlyName(), (*it)->getTypeSz(), (*it)->isAvailable()?"yes":"no", (*it)->isRetainedCmd()?"yes":"no", (*it)->getTopicBase(), (*it)->hasCb()?"yes":"no", (*it)->hasCmdIF()?"yes":"no"});
         it++;
         n++;
      }
   }
      
};


class CxMqttHASensor : public CxMqttHABase {
// https://www.home-assistant.io/integrations/sensor.mqtt/
private:
   const char* _szDeviceClass;
   const char* _szUnit;
   CxSensor* _pSensor;
   
   CxTimer _timer;
   
public:
   
   CxMqttHASensor(CxSensor* pSensor, uint32_t nPeriod) : CxMqttHASensor(pSensor->getFriendlyName(), pSensor->getName(), pSensor->getTypeSz(), pSensor->getUnit()) {_pSensor = pSensor; _timer.start(nPeriod);}
      
   CxMqttHASensor(const char* fn, const char* name, const char* dclass = nullptr, const char* unit = nullptr, bool available = false, bool retain = false) : CxMqttHABase(fn, name, nullptr, nullptr, nullptr, retain), _szDeviceClass(dclass), _szUnit(unit), _pSensor(nullptr) {
      
      __eStateClass = e_state::measurement;
      __eCat = e_cat::none;
      __eType = e_type::HAsensor;
      setAvailable(available);
   }
   
   CxSensor* getSensor() const {return _pSensor;}
   
   bool isDue() {return _timer.isDue();}
   
   void addJsonConfig(JsonDocument &doc) const {
      if (_szDeviceClass) {
         doc[F("dev_cla")] = _szDeviceClass;
      }
      if (_szUnit) {
         doc[F("unit_of_meas")] = _szUnit;
      }
   }
   void addJsonAction(JsonDocument& doc) const {}
   
   
};

class CxMqttHAButton : public CxMqttHABase {
//https://www.home-assistant.io/integrations/button.mqtt/

   CxGPIODevice* _pButton;
   
public:
   
   // constructor for gpio button devices. set availability to true, as the gpio button shal be available by default.
   CxMqttHAButton(CxGPIODevice* pButton) : CxMqttHAButton(pButton->getFriendlyName(), pButton->getName(), true, nullptr, nullptr, false) {_pButton = pButton;}
   
   CxMqttHAButton(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, const char* topic = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, topic, retain) {
      
      __eCat = e_cat::none; // default
      __eType = e_type::HAsensor;  // a physical button is a sensor
      __szAction = "action_single";

      if (isAction()) setActionTopic();
      setAvailable(available);
   }
   
   void addJsonConfig(JsonDocument &doc) const {
      
      doc[F("en")] = true;
      doc[F("ic")] = F("mdi:gesture-double-tap");
      
   }
   
   void addJsonAction(JsonDocument& doc) const {
      // TODO: if more than one action is defined in a device, than the subtype need to be different from each other!
      doc[F("subtype")] = F("single");
      doc[F("payload")] = F("single");
      
   }
   
};

class CxMqttHAText : public CxMqttHABase {
// https://www.home-assistant.io/integrations/text.mqtt/
private:
   uint32_t _nMax;
   String _strFriendlyName;

public:
   
   CxMqttHAText(const char* fn, const char* name, uint32_t max, bool available = false, CxMqttManager::tCallback cb = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, nullptr, retain), _nMax(max) {
      
      __eCat = e_cat::none; // default
      __eType = e_type::HAtext;
      __bCmd = true;
      setAvailable(available);
      
   }
   CxMqttHAText(const char* szName) : CxMqttHAText(szName, szName, 64, true) {}
   
   void setFn(const char* fn) {_strFriendlyName = fn; setFriendlyName(_strFriendlyName.c_str());}
   void setMax(uint32_t set) {_nMax = set;}
   
   void addJsonConfig(JsonDocument &doc) const {
      
      doc[F("ic")] = F("mdi:ab-testing");
      doc[F("mode")] = F("text");
      doc[F("max")] = _nMax;
      
   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHALight : public CxMqttHABase {
// https://www.home-assistant.io/integrations/light.mqtt/
private:
   
public:
   
   CxMqttHALight(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, nullptr, retain) {
      
      __eCat = e_cat::none; // default
      __eType = e_type::HAlight;
      __bCmd = true;
      setAvailable(available);
      
   }
   
   void addJsonConfig(JsonDocument &doc) const {}
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHASwitch : public CxMqttHABase {
// https://www.home-assistant.io/integrations/switch.mqtt/
private:
   
   CxGPIODevice* _pSwitch;
   
public:
   
   // constructor for gpio switch devices. set availability to true, as the gpio switch shal be available by default.
   CxMqttHASwitch(CxGPIODevice* pSwitch, CxMqttManager::tCallback cb = nullptr) : CxMqttHASwitch(pSwitch->getFriendlyName(), pSwitch->getName(), true, cb, false) {_pSwitch = pSwitch;}
   
   CxMqttHASwitch(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, nullptr, retain) {
      
      __eCat = e_cat::none; // default
      __eType = e_type::HAswitch;
      __bCmd = true;
      setAvailable(available);
   }
   
   void addJsonConfig(JsonDocument &doc) const {}
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHADiagnostic : public CxMqttHASensor {
private:
   const char* _szTopicState;
   bool _bUseTopicState; // instead of using json state in ~/state use non-json ~/ or alternative topic state if given in _szTopicState
   
public:
   
   // constructor for diagnostic "online/offline" status entities
   CxMqttHADiagnostic(const char* fn, const char* name, bool bUseTopicState = false, const char* topicState = nullptr, bool retain = false) : CxMqttHASensor(fn, name, nullptr, nullptr, true, retain), _szTopicState(topicState), _bUseTopicState(bUseTopicState) {
      
      __eCat = e_cat::diagnostic;
      setAvailable(false); // by default offline
      
   }
   // constructor for diagnostic measurement entities
   CxMqttHADiagnostic(const char* fn, const char* name, const char* dclass = nullptr, const char* unit = nullptr, bool retain = false) : CxMqttHASensor(fn, name, dclass, unit, true, retain), _szTopicState(nullptr), _bUseTopicState(false) {
      
      __eCat = e_cat::diagnostic;
      setAvailable(true);


   };

   
   void addJsonConfig(JsonDocument &doc) const {
      
      CxMqttHASensor::addJsonConfig(doc);
      
      // overwrite "stat_t" and remove its template, as it is not a json
      if (_bUseTopicState) {
         doc[F("stat_t")] = _szTopicState ? _szTopicState : "~";
         doc.remove(F("val_tpl"));
      }
   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHABinarySensor : public CxMqttHABase {
// https://www.home-assistant.io/integrations/binary_sensor.mqtt/
private:
   const char* _szDeviceClass;
   
public:
   
   CxMqttHABinarySensor(const char* fn, const char* name, const char* dclass = nullptr, bool retain = false, bool available = false) : CxMqttHABase(fn, name, nullptr, nullptr, nullptr, retain), _szDeviceClass(dclass) {
      
      __eCat = e_cat::none;
      __eType = e_type::HAbinary;
      setAvailable(available);
   }
   
   void addJsonConfig(JsonDocument &doc) const {
      if (_szDeviceClass) doc[F("dev_cla")] = _szDeviceClass;
   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHANumber : public CxMqttHABase {
// https://www.home-assistant.io/integrations/number.mqtt/
private:
   int32_t _nMin;
   int32_t _nMax;
   int32_t _nStep;
   
   const char* _szDeviceClass;
   String _strUnit;
   String _strFriendlyName;
      
public:
   
   CxMqttHANumber(const char* fn, const char* name, const char* dclass = nullptr, bool available = false, CxMqttManager::tCallback cb = nullptr, int32_t min = 0, int32_t max = 100, int32_t step = 1, const char* unit = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, nullptr, retain), _nMin(min), _nMax(max), _nStep(step), _strUnit(unit), _szDeviceClass(dclass) {
      
      __eCat = e_cat::none;
      __eType = e_type::HAnumber;
      __bCmd = true;
      setAvailable(available);
   }
   CxMqttHANumber(const char* szName) : CxMqttHANumber(szName, szName, nullptr, true) {}
   
   void setMin(int32_t set) {_nMin = set;}
   void setMax(int32_t set) {_nMax = set;}
   void setStep(int32_t set) {_nStep = set;}
   void setUnit(const char* set) {_strUnit = set;}
   void setFn(const char* fn) {_strFriendlyName = fn; setFriendlyName(_strFriendlyName.c_str());}

   void addJsonConfig(JsonDocument &doc) const {
      doc[F("min")] = _nMin;
      doc[F("max")] = _nMax;
      doc[F("step")] = _nStep;
      if (_szDeviceClass) doc[F("dev_cla")] = _szDeviceClass;
      if (_strUnit.length()) doc[F("unit_of_meas")] = _strUnit.c_str();

   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

#ifndef MINIMAL_COMMAND_SET

class CxMqttHASiren : public CxMqttHABase {
   // https://www.home-assistant.io/integrations/siren.mqtt/
private:
  
   
public:
   
   CxMqttHASiren(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, nullptr, retain)  {
      
      __eCat = e_cat::none;
      __eType = e_type::HAsiren;
      __bCmd = true;
      setAvailable(available);
   }
   
   void addJsonConfig(JsonDocument &doc) const {
      JsonArray array = doc.createNestedArray(F("av_tones"));
      array[0] = "ping";
      array[1] = "siren";
      array[2] = "dingdong";
      array[3] = "attention";

      doc[F("sup_dur")] = true;
      doc[F("sup_vol")] = true;
      
   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHAAlarmPanel : public CxMqttHABase {
   //https://blog.cavelab.dev/2021/11/home-assistant-manual-alarm/
private:
   
public:
   CxMqttHAAlarmPanel(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, const char* topic = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, topic, retain) {
      __eCat = e_cat::none;
      __eType = e_type::HAalarmpanel;
      __bCmd = true;
      setAvailable(available);

   }
   
   void addJsonConfig(JsonDocument &doc) const {
      doc[F("code")] = "2801";
   }
   
   void addJsonAction(JsonDocument& doc) const {}

};

class CxMqttHANotify : public CxMqttHABase {
   //https://www.home-assistant.io/integrations/notify.mqtt/
private:
   
public:
   CxMqttHANotify(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb, nullptr, retain) {
      __eCat = e_cat::none;
      __eType = e_type::HAnotify;
      __bCmd = true;
      setAvailable(available);
   }
   
   void addJsonConfig(JsonDocument &doc) const {}
   
   void addJsonAction(JsonDocument& doc) const {}
   
};

class CxMqttHAEvent : public CxMqttHABase {
   //https://www.home-assistant.io/integrations/event.mqtt/
private:
   
public:
   CxMqttHAEvent(const char* fn, const char* name, bool available = false, bool retain = false) : CxMqttHABase(fn, name, nullptr, nullptr, nullptr, retain) {
      __eCat = e_cat::none;
      __eType = e_type::HAevent;
      setAvailable(available);
   }
   
   void addJsonConfig(JsonDocument &doc) const {
      JsonArray array = doc.createNestedArray(F("evt_typ"));
      array[0] = "myevent";
      doc.remove(F("val_tpl")); // remove template, HA is looking for {"event_type": "myevent", <further attrib>}
   }
   
   void addJsonAction(JsonDocument& doc) const {}
   
};
#endif

class CxMqttHASelect : public CxMqttHABase {
   //https://www.home-assistant.io/integrations/select.mqtt/
private:
   std::vector<String> _vOptions;
   uint8_t _nOption;
   String _strFriendlyName;
   
public:
   CxMqttHASelect(const char* fn, const char* name, bool available = false, CxMqttManager::tCallback cb = nullptr, const std::vector<String>* pvOpt = nullptr, bool retain = false) : CxMqttHABase(fn, name, nullptr, cb), _nOption(0) {
      if (pvOpt) _vOptions = *pvOpt;
      __eCat = e_cat::none;
      __eType = e_type::HAselect;
      __bCmd = true;
      setAvailable(available);
   }
   CxMqttHASelect(const char* name, const std::vector<String>& vOpt, CxMqttManager::tCallback cb = nullptr) : CxMqttHASelect("", name, true, cb, &vOpt) {}
   
   uint8_t getOption(uint8_t* payload, unsigned int len) {
      _nOption = 0;
      for (size_t i = 0; i < _vOptions.size(); ++i) {
         if (std::strcmp(_vOptions[i].c_str(), (char*) payload) == 0)
            return static_cast<int>(i+1); // 1-based return
      }
      return getOption(); // 0: option not found/set
   }
   
   void addOption(const char* szOpt) {
      if (!szOpt) return;
      _vOptions.push_back(szOpt);
   }
   
   void setFn(const char* fn) {_strFriendlyName = fn; setFriendlyName(_strFriendlyName.c_str());}
   
   uint8_t getOption() {return _nOption;}
   void setOption(uint8_t set) {_nOption = set;}
   
   const char* getOptionStr(uint8_t nOpt = 0) {
      if (!nOpt) nOpt = _nOption;
      
      if (nOpt && nOpt <= _vOptions.size()) {
         return _vOptions[nOpt-1].c_str();
      }
      return "";
   }
   
   //void publishState() {CxMqttHABase::publishState(getOptionStr().c_str());}
      
   void addJsonConfig(JsonDocument &doc) const override {
      JsonArray arr = doc.createNestedArray("options");
      for (const auto& option : _vOptions) {
         arr.add(option.c_str());
      }
      //doc[F("icon")] = "mdi:palette";
 
   }
  
   void addJsonAction(JsonDocument& doc) const override {}
   
};

#endif /* CxMqttHA_hpp */
