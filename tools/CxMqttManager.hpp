/**
 * @file CxMqttManager.hpp
 * @brief MQTT Manager for handling MQTT communication on ESP-based devices.
 *
 * ## CxMqttManager
 * This class provides a singleton-based MQTT client manager for handling MQTT connections,
 * subscribing to topics, and publishing messages. It abstracts the underlying WiFi client
 * and PubSubClient to facilitate easy integration with MQTT-based communication in IoT applications.
 *
 * ## Features:
 * - Manages MQTT connection lifecycle, including automatic reconnection.
 * - Supports subscription and unsubscription of topics with callback functions.
 * - Handles message publishing, including retained messages and last will messages.
 * - Supports relative and absolute topic paths for flexible topic management.
 * - Provides configurable MQTT server settings (address, port, QoS, buffer size).
 * - Implements a structured callback mechanism to handle incoming messages efficiently.
 *
 * ## Usage:
 * - Initialize the manager with `CxMqttManager::getInstance().begin(server, port, qos);`
 * - Subscribe to topics using `subscribe(topic, callback);`
 * - Publish messages using `publish(topic, message, retain);`
 * - Maintain the connection using `loop();`
 * - Automatically reconnects if the connection is lost (configurable).
 *
 * ## CxMqttTopicBase
 * The CxMqttTopicBase class provides an abstraction for managing individual MQTT topics,
 * allowing for easy publishing, subscribing, and handling of messages through callbacks.
 * It integrates with CxMqttManager for centralized MQTT communication.
 *
 * ## Features:
 * - Stores topic name, QoS level, and retained message flag.
 * - Supports setting and clearing retained messages.
 * - Allows publishing to a specific topic.
 * - Manages subscriptions with callback functions.
 *
 * ## CxMqttTopic
 * The derived class CxMqttTopic extends CxMqttTopicBase and automatically subscribes to the
 * assigned topic upon instantiation if auto-subscribe is enabled.
 *
 * ## Usage Example
 * Simplified code without WiFi setup, which is needed.
 *
 * ```cpp
 * #include "CxMqttManager.h"
 *
 * CxMqttManager& mqttManager = CxMqttManager::getInstance();
 *
 * CxMqttTopic myTopic("ExampleTopic", "home/sensor/temp",
 *     [](const char* topic, const char* message) {
 *         Serial.printf("Received on %s: %s\n", topic, message);
 *     }
 * );
 *
 * void setup() {
 *     Serial.begin(115200);
 *     mqttManager.begin("mqtt_broker_address", 1883); // Initialize MQTT manager
 *     myTopic.publish("22.5"); // Publish a temperature reading
 * }
 *
 * void loop() {
 *     mqttManager.loop(); // Ensure MQTT messages are processed
 * }
 * ```
 *
 * ## Issues:
 * - Missing Mutex for Thread Safety
 * - Lack of TLS Support
 * - incorrect Singleton Pattern:  The class uses static CxMqttManager instance; in getInstance(), which is correct, but the constructor initializes _mqttClient. This might not work well if the order of initialization is undefined.
 *
 *
 * @author ocfu
 * @date 26.01.2025
 */

#ifndef CxMqttManager_hpp
#define CxMqttManager_hpp

#include <map>
#include <functional>
#ifdef ARDUINO
#include <PubSubClient.h>
#endif
#include "esphw.h"  // for getChidId() to set the clientId
#include "CxESPConsole.hpp"

#ifdef ESP_CONSOLE_NOWIFI
#error "ESP_CONSOLE_NOWIFI was defined. MQTT requires a network to work!"
#endif

/**
 * @class MQTTManager
 * @brief A class to manage MQTT connections and topic subscriptions using PubSubClient.
 *
 * ## Issues:
 * - Missing Mutex for Thread Safety
 * - Lack of TLS Support
 */
class CxMqttManager {
public:
   using tCallback = std::function<void(const char*, uint8_t*, unsigned int)>;

private:
   CxESPConsoleMaster& console = CxESPConsoleMaster::getInstance();

   bool         _bIsInitialized; ///< the mqtt manager is ready to use
   WiFiClient   _wifiClient;    ///< WiFi client for underlying network communication.
   PubSubClient _mqttClient;    ///< MQTT client using the WiFi client.
   String       _strClientId;   ///< Client ID for the MQTT connection
   
   std::map<const char*, std::pair<int, tCallback>, std::less<>> _mapTopicCallbacks; ///< Map of topics and their respective callback functions.
   std::map<const char*, bool> _mapIsRelative;
   
   String   _strServer;                  ///< MQTT server address.
   uint16_t _nPort;                      ///< MQTT server port.
   uint8_t  _nQoS;                       ///< Quality of Service level.
   String   _strRootPath;                ///< Root path for topics.
   bool     _bReconnect;                 ///< reconnect attempt in the loop, if connection gets lost
   uint32_t _nLastReconnectAttempt;      ///< Timestamp of the last reconnection attempt.
   uint16_t _nBufferSize;                ///< buffer size for pusbuclient
   String   _strWillTopic;
   String   _strWillMessage;
   bool     _bWill;
   uint32_t _nConnectCntr;
   
   /**
    * @brief Generates a randomized client ID for the MQTT connection.
    * @return A string containing the randomized client ID.
    */
   String _generateClientId() {
      String clientId = "esp";
      clientId += getChipId();
      return clientId;
   }
   
   /**
    * @brief Resubscribes to all previously subscribed topics.
    */
   void _resubscribeTopics() {
      _CONSOLE_DEBUG(F("re-subscribe all topics"));

      for (const auto& pair : _mapTopicCallbacks) {
         String strTopic;
         if (_mapIsRelative[pair.first]) {
            strTopic = _strRootPath + '/' + pair.first;
         } else {
            strTopic = pair.first+1;
         }
         _CONSOLE_DEBUG(F("re-subscribe topic %s"), strTopic.c_str());

         _mqttClient.subscribe(strTopic.c_str(), _nQoS);
      }
   }
   
public:
   /**
    * @brief Constructor initializing default MQTT parameters.
    *
    * ## Issues:
    * incorrect Singleton Pattern:  The class uses static CxMqttManager instance; in getInstance(), which is correct, but the constructor initializes _mqttClient. This might not work well if the order of initialization is undefined.
    *
    * Fix: Explicitly initialize _mqttClient only when begin() is called.
    */
   CxMqttManager() : _mqttClient(_wifiClient), _nPort(1883), _nQoS(0), _nLastReconnectAttempt(0),
   _nBufferSize(128), _bReconnect(true), _strWillMessage(F("offline")), _bWill(false), _nConnectCntr(0), _bIsInitialized(true) {
      _strClientId = _generateClientId();
   }
   
   /**
    * @brief Access the singleton instance of CxMQTTManager.
    * @return Reference to the singleton instance.
    */
   static CxMqttManager& getInstance() {
      static CxMqttManager instance; // Constructed on first access
      return instance;
   }
   
   // Disable copying and assignment to enforce singleton pattern
   CxMqttManager(const CxMqttManager&) = delete;
   CxMqttManager& operator=(const CxMqttManager&) = delete;
   
   bool isIntitialized() {return _bIsInitialized;}
   
   /**
    * @brief Sets the MQTT server address.
    * @param serverAddr The server address.
    */
   void setServer(const char* serverAddr) {
      _strServer = serverAddr;
   }
   
   /**
    * @brief Gets the MQTT server address.
    * @return The server address as a C-string.
    */
   const char* getServer() {
      return _strServer.c_str();
   }
   
   /**
    * @brief Sets the MQTT server port.
    * @param serverPort The server port.
    */
   void setPort(int serverPort) {
      _nPort = serverPort;
   }
   
   /**
    * @brief Gets the MQTT server port.
    * @return The server port.
    */
   uint16_t getPort() {
      return _nPort;
   }
   
   /**
    * @brief Sets the Quality of Service (QoS) level.
    * @param qualityOfService The QoS level (0, 1, or 2).
    */
   void setQoS(int qualityOfService) {
      _nQoS = qualityOfService;
   }
   
   /**
    * @brief Gets the Quality of Service (QoS) level.
    * @return The QoS level.
    */
   uint8_t getQoS() {
      return _nQoS;
   }
   
   /**
    * @brief Sets the root path for topics.
    * @param path The root path.
    */
   void setRootPath(const char* path) {
      if (_strRootPath != path) {
         _CONSOLE_DEBUG(F("set new root path to %s"), path);

         // root path has changed, re-subscribe all subscribtions with a relative path
         for (const auto& pair : _mapTopicCallbacks) {
            if (_mapIsRelative[pair.first]) {
               String strTopic = _strRootPath + '/' + pair.first;
               _CONSOLE_DEBUG(F("unsubscribe topic %s"), strTopic.c_str());
               _mqttClient.unsubscribe(strTopic.c_str());
               
               strTopic = path;
               strTopic += '/';
               strTopic += pair.first;
               _CONSOLE_DEBUG(F("subscribe topic %s"), strTopic.c_str());
               _mqttClient.subscribe(strTopic.c_str());
            }
         }
         _strRootPath = path;
      }
   }
   
   /**
    * @brief Gets the root path for topics.
    * @return The root path as a C-string.
    */
   const char* getRootPath() {
      return _strRootPath.c_str();
   }
   
   void setBufferSize(uint16_t size) {
      _nBufferSize = (size > 128) ? size : 128;
   }
   
   uint16_t getBufferSize() {
      return _nBufferSize;
   }
   
   void setReconnect(bool set) {_bReconnect = set;}
   
   void setWill(bool set) {_bWill = set;}  // will message will be send to the root path, if no will topic is set
   bool isWill() {return _bWill;}
   
   void setWillTopic(const char* sz) {
      if (sz) _strWillTopic = sz; else _strWillTopic = "";
      if (_strWillTopic.length()) {
         _strWillTopic.trim();
         if (_strWillTopic.startsWith("/")) {
            _strWillTopic.remove(0);
         }
      }
      if (!_bWill) _bWill = (_strWillTopic.length() > 0);
   }
   const char* getWillTopic() {return _strWillTopic.c_str();}
   
   void setWillMessage(const char* sz) {if (sz) _strWillMessage = sz; else _strWillTopic = "";}
   const char* getWillMessage() {return _strWillMessage.c_str();}
   
   void publishWill(const char* msg = nullptr) {
      if (_bWill) {
         publish(_strWillTopic.c_str(), (msg) ? msg : _strWillMessage.c_str(), true);
      }
   }
   
   /**
    * @brief Initializes the MQTT client with the stored server and port.
    */
   bool begin(const char* s = nullptr, int p = 0, int q = 0) {
      bool bResult = false;
      end(_bReconnect); // keep the reconnect behaviour
      if (s) setServer(s);
      if (p) setPort(p);
      if (q) setQoS(q);
      if (_strServer.length()) {
         _mqttClient.setServer(_strServer.c_str(), _nPort);
         _mqttClient.setBufferSize(_nBufferSize);
         _mqttClient.setCallback([this](const char* topic, uint8_t* payload, unsigned int length) {
            if (topic && payload) {
               payload[length] = '\0';
               _CONSOLE_DEBUG(F("received from topic %s: '%s'"), topic, (char*)payload);

               for (const auto& pair : _mapTopicCallbacks) {
                  if (!pair.second.second) continue; // has no callback
                  if (_mapIsRelative[pair.first]) {
                     String strTopic = _strRootPath + '/' + pair.first;
                     _CONSOLE_DEBUG(F("compare topics '%s' with '%s'"), strTopic.c_str(), topic);

                     if (strTopic == topic) pair.second.second(topic, payload, length);
                  } else {
                     _CONSOLE_DEBUG(F("compare topics '%s' with '%s'"), pair.first+1, topic);
                     if (strcmp(pair.first+1, topic) == 0) { // without heading '/'
                        pair.second.second(topic, payload, length);
                     }
                  }
               }
            }
         });
         _bIsInitialized = true;
         bResult = connect();
      }
#ifdef ARDUINO
      yield(); // Mqtt could took some time, feed the watchdog
#endif
      return bResult;
   }
   
   /**
    * @brief Initializes the MQTT client with a specified server and root path.
    * @param serverAddr The server address.
    * @param rootPath The root path (default: empty string).
    */
   void begin(const char* serverAddr, const char* rootPath = "") {
      setServer(serverAddr);
      setRootPath(rootPath);
      begin();
   }
   
   /**
    * @brief Disconnects the MQTT client and sets reconnect
    */
   void end(bool bReconnect = true) {
      if (isConnected()) {
         publishWill(); // a disconnect seems not triggering the mqtt server to publish the will message
         _mqttClient.disconnect();
         _bIsInitialized = false;
      }
      _bReconnect = bReconnect;
   }
   
   /**
    * @brief Handles MQTT tasks and attempts reconnection if disconnected.
    */
   void loop() {
      if (_bReconnect && !_mqttClient.connected()) {
         uint32_t now = (uint32_t)millis();
         if (now - _nLastReconnectAttempt > 60000) {
            _nLastReconnectAttempt = now;
            connect();
         }
      } else {
         _mqttClient.loop();
      }
   }
   
   /**
    * @brief Checks if the MQTT client is connected.
    * @return True if connected, otherwise false.
    */
   bool isConnected() {
      return _mqttClient.connected();
   }
   
   /**
    * @brief Connects to the MQTT server with a client ID and last will message.
    * @return True if the connection is successful, otherwise false.
    */
   bool connect() {
      bool bConnected = false;
      if (_bWill) {
         // if defined, set retained last will topic with message and QoS=1
         if (_strWillTopic.length()) {
            bConnected = _mqttClient.connect(_strClientId.c_str(), NULL, NULL, (_strRootPath + "/" + _strWillTopic).c_str(), 1, true, _strWillMessage.c_str());
         } else {
            bConnected = _mqttClient.connect(_strClientId.c_str(), NULL, NULL, _strRootPath.c_str(), 1, true, _strWillMessage.c_str());
         }
      } else {
         bConnected = _mqttClient.connect(_strClientId.c_str());
      }
      if (bConnected) {
         _nConnectCntr++;
         _resubscribeTopics();
      }
      return bConnected;
   }
   
   uint32_t getConnectCntr() {return _nConnectCntr;}
   
   /**
    * @brief Publishes a message to a topic.
    * @param topic The topic path.
    * @param value The message value.
    * @param retain Whether the message should be retained.
    * @return True if the message is published successfully, otherwise false.
    */
   bool publish(const char* topic, const char* value, bool retain = false) {
      // is topic given as an absolute path (indicated with a starting '/'), than ignore the root path.
      if (topic && topic[0] == '/' && topic[1]) {
         return _mqttClient.publish(topic+1, value, retain);
      } else if (topic && topic[0]){
         return _mqttClient.publish((_strRootPath + '/' + topic).c_str(), value, retain);
      } else {
         return _mqttClient.publish(_strRootPath.c_str(), value, retain);
      }
   }
   
   /**
    * @brief Subscribes to a topic with a callback function.
    * @param topic The topic to subscribe to.
    * @param callback The callback function to handle messages.
    * @return True if the subscription is successful, otherwise false.
    */
   bool subscribe(const char* topic, tCallback callback) {
      if (!topic || strlen(topic) < 1) return false;
      static int callbackId = 0; // Incremental unique ID to identify the callback later
      bool bIsRelative = (topic[0] != '/');
      if (bIsRelative && strlen(topic) < 2) return false;
      _mapTopicCallbacks[topic] = {++callbackId, callback};
      _mapIsRelative[topic] = bIsRelative;
      String strTopic;
      if (bIsRelative) {
         strTopic = _strRootPath + '/' + topic;
      } else {
         strTopic = ++topic; // without heading '/'
      }
      _CONSOLE_DEBUG(F("subscribe topic %s"), strTopic.c_str());
      return _mqttClient.subscribe(strTopic.c_str(), _nQoS);
   }
   
   /**
    * @brief Unsubscribes from a topic.
    * @param topic The topic to unsubscribe from.
    * @return True if the unsubscription is successful, otherwise false.
    */
   bool unsubscribe(const char* topic) {
      if (!topic || strlen(topic) < 1) return false;
      _mapTopicCallbacks.erase(topic);
      bool bIsRelative = (topic[0] != '/');
      String strTopic;
      if (bIsRelative) {
         strTopic = _strRootPath + '/' + topic;
         _mapIsRelative.erase(topic);
      } else {
         strTopic = ++topic; // without heading '/'
      }
      _CONSOLE_DEBUG(F("unsubscribe topic %s"), strTopic.c_str());
      return _mqttClient.unsubscribe(strTopic.c_str());
   }
   
   /**
    * @brief Finds the callback function for a topic.
    * @param topic The topic to search for.
    * @return The callback function if found, otherwise nullptr.
    */
   tCallback findTopic(const char* topic) {
      for (const auto& pair : _mapTopicCallbacks) {
         if (strcmp(pair.first, topic) == 0) {
            return pair.second.second;
         }
      }
      return nullptr;
   }
   
   /**
    * @brief Checks if a callback function exists in the map.
    * @param callback The callback function to search for.
    * @return True if the callback function is found, otherwise false.
    */
   // TODO: this does not work !!!
   bool findCb(tCallback callback) {
      for (const auto& pair : _mapTopicCallbacks) {
         if (pair.second.first == reinterpret_cast<intptr_t>(&callback)) {
            return true;
         }
      }
      return false;
   }
   
   /**
    * @brief Removes a topic and its callback function.
    * @param topic The topic to remove.
    */
   void removeTopic(const char* topic) {
      unsubscribe(topic);
   }
   
   void printSubscribtion(Stream& stream) {
      for (const auto& pair : _mapTopicCallbacks) {
         if (pair.first) {
            String strTopic;
            if (_mapIsRelative[pair.first]) {
               strTopic = _strRootPath + '/' + pair.first;
            } else {
               strTopic = pair.first+1; // without heading '/'
            }
            stream.println(strTopic.c_str());
         }
      }
   }
   
};


/**
 * @brief Base class for managing MQTT topics in an Arduino/ESP environment.
 *
 * This class provides core functionality for handling MQTT topics, including
 * publishing, subscribing, and managing retained messages.
 *
 * ## Issues:
 * Lack of Thread Safety
 * Issue:  _mqttManager is accessed without synchronization. If multiple threads access publish(), subscribe(), or unsubscribe(), race conditions may occur.
 *
 * Recommendation:  If running in a multithreaded environment, improve by using a mutex or other synchronization method.
 * Example:
 * #include <mutex>
 * std::mutex mqttMutex;
 *
 * void subscribe() {
 *  if (!_cb) return;
 *  std::lock_guard<std::mutex> lock(mqttMutex);
 *  _mqttManager.subscribe(_strTopic.c_str(), _cb);
 * }
 *
 * void unsubscribe() {
 *  std::lock_guard<std::mutex> lock(mqttMutex);
 *  _mqttManager.unsubscribe(_strTopic.c_str());
 * }
 *
 */
class CxMqttTopicBase {
private:
   String _strTopic;          ///< Stores the MQTT topic string.
   bool _bRetained;           ///< Retained message flag.
   uint8_t _nQos;             ///< QoS level (0, 1, or 2).
   CxMqttManager& _mqttManager = CxMqttManager::getInstance(); ///< MQTT manager instance.
   CxMqttManager::tCallback _cb; ///< Callback function for received messages.

public:
   /**
    * @brief Constructor to initialize the MQTT topic.
    *
    * @param topic MQTT topic string.
    * @param cb Callback function for received messages (optional).
    * @param retain Whether to retain messages (default: false).
    */
   CxMqttTopicBase(const char* topic , CxMqttManager::tCallback cb = nullptr, bool retain = false) : _bRetained(retain), _nQos(0), _strTopic(topic) {
      setCb(cb);
   };
   
   CxMqttTopicBase() : _bRetained(false), _nQos(0) {};

   
   /**
    * @brief Destructor unsubscribes from the topic.
    */
   ~CxMqttTopicBase() {unsubscribe();}
   
   void setCb( CxMqttManager::tCallback cb) {_cb = cb;} ///< Sets the callback function.
   bool hasTopic() { return (_strTopic.length());} ///< Checks if the topic is set.
   
   const char* getTopic() {return _strTopic.c_str();} ///< Retrieves the topic string.
   
   void setTopic(const char* topic) {
      /// FIXME: handle subscribtion of the topic, if it has changed.
      _strTopic = topic;
   }
   
   const char* getRootPath() {
      if (_mqttManager.isIntitialized()) {
         return _mqttManager.getRootPath();
      } else {
         return "";
      }
   }
   
   void setRetained(bool set) {_bRetained = set;} ///< Sets the retained message flag.
   bool isRetained() {return _bRetained;} ///< Checks if the message is retained.
   bool clearRetainedMessage(const char* topic = nullptr) { ///< Clears retained messages.
      if (topic && strlen(topic) > 0) {
         return publish(topic, "", true);
      }
      if (_strTopic.length() > 0) {
         return publish(_strTopic.c_str(), "", true);
      }
      return false; // Avoid publishing to an empty topic
   }
   
   void setQos(uint8_t qos) { ///< Sets the QoS level.
      if (qos < 3) _nQos = qos;
   }
   
   bool publish(const char* payload, bool retained = false) { return publish(_strTopic.c_str(), payload, retained); } ///< Publishes a message to the set topic.
   bool publish(const char* topic, const char* payload, bool retained = false) { ///< Publishes to a specific topic.
      if (_mqttManager.isIntitialized()) {
         return _mqttManager.publish(topic, payload, retained);
      } else {
         return false;
      }
   }
   
   void subscribe() { ///< Subscribes to the topic (if a callback is set).
      if (! _cb) {
         return;
      }
      if (_mqttManager.isIntitialized()) {
         _mqttManager.subscribe(_strTopic.c_str(), _cb);
      }
   }
   
   void unsubscribe() { ///< Unsubscribes from the topic.
      if (_mqttManager.isIntitialized()) {
         _mqttManager.unsubscribe(_strTopic.c_str());
      }
   }
   
};

/**
 * @brief Derived class that automatically subscribes to the MQTT topic.
 *
 * This class extends CxMqttTopicBase and subscribes to the topic upon instantiation.
 */
class CxMqttTopic : public CxMqttTopicBase {
public:
   /**
    * @brief Constructor that initializes and subscribes to the topic.
    *
    * @param topic MQTT topic string.
    * @param cb Callback function for received messages (optional).
    * @param retain Whether to retain messages (default: false).
    * @param autoSubscribe Whether to subscribe immidiately to the set topic (default: true).
    */
   CxMqttTopic(const char* topic, CxMqttManager::tCallback cb = nullptr, bool retain = false, bool autoSubscribe = true) : CxMqttTopicBase(topic, cb, retain) {if (autoSubscribe) subscribe();}
   
   CxMqttTopic() : CxMqttTopicBase() {};
};


#endif /* CxMqttManager_hpp */
