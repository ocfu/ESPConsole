//
//  CxMqttManager.hpp
//
//
//  Created by ocfu on 15.01.24.
//

#ifndef CxMqttManager_hpp
#define CxMqttManager_hpp

#include <map>
#include <functional>
#ifdef ARDUINO
#include <PubSubClient.h>
#endif
#include "esphw.h"  // for getChidId() to set the clientId

#ifdef ESP_CONSOLE_NOWIFI
#error "ESP_CONSOLE_NOWIFI was defined. MQTT requires a network to work!"
#endif

/**
 * @class MQTTManager
 * @brief A class to manage MQTT connections and topic subscriptions using PubSubClient.
 */
class CxMqttManager {
private:
   
   WiFiClient   _wifiClient;    ///< WiFi client for underlying network communication.
   PubSubClient _mqttClient;    ///< MQTT client using the WiFi client.
   String       _strClientId;   ///< Client ID for the MQTT connection
   
   std::map<const char*, std::pair<int, std::function<void(const char*, uint8_t*, unsigned int)>>, std::less<>> _mapTopicCallbacks; ///< Map of topics and their respective callback functions.
   
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
      for (const auto& pair : _mapTopicCallbacks) {
         _mqttClient.subscribe(pair.first, _nQoS);
      }
   }
   
public:
   /**
    * @brief Constructor initializing default MQTT parameters.
    */
   CxMqttManager() : _mqttClient(_wifiClient), _nPort(1883), _nQoS(0), _nLastReconnectAttempt(0),
   _nBufferSize(128), _bReconnect(true), _strWillMessage(F("offline")), _bWill(false), _nConnectCntr(0) {
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
      _strRootPath = path;
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
      _strWillTopic = sz;
      _strWillTopic.trim();
      if (_strWillTopic.startsWith("/")) {
         _strWillTopic.remove(0);
      }
      if (!_bWill) _bWill = (_strWillTopic.length() > 0);
   }
   const char* getWillTopic() {return _strWillTopic.c_str();}
   
   void setWillMessage(const char* sz) {_strWillMessage = sz;}
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
               
               for (const auto& pair : _mapTopicCallbacks) {
                  if (strcmp(pair.first, topic) == 0) {
                     if (pair.second.second) pair.second.second(topic, payload, length);
                  }
               }
            }
         });
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
            if (connect()) {
               _resubscribeTopics();
            }
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
      if (bConnected) _nConnectCntr++;
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
         return _mqttClient.publish((_strRootPath + "/" + topic).c_str(), value, retain);
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
   bool subscribe(const char* topic, std::function<void(const char*, uint8_t*, unsigned int)> callback) {
      static int callbackId = 0; // Incremental unique ID
      _mapTopicCallbacks[topic] = {++callbackId, callback};
      return _mqttClient.subscribe(topic, _nQoS);
   }
   
   /**
    * @brief Unsubscribes from a topic.
    * @param topic The topic to unsubscribe from.
    * @return True if the unsubscription is successful, otherwise false.
    */
   bool unsubscribe(const char* topic) {
      _mapTopicCallbacks.erase(topic);
      return _mqttClient.unsubscribe(topic);
   }
   
   /**
    * @brief Finds the callback function for a topic.
    * @param topic The topic to search for.
    * @return The callback function if found, otherwise nullptr.
    */
   std::function<void(char*, uint8_t*, unsigned int)> findTopic(const char* topic) {
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
   bool findCb(std::function<void(char*, uint8_t*, unsigned int)> callback) {
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
            stream.println(pair.first);
         }
      }
   }
   
};

#endif /* CxMqttManager_hpp */
