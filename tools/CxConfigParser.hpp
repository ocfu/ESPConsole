//
//  CxConfigParser.hpp
//
//
//  Created by ocfu on 15.01.24.
//

#ifndef CxConfigParser_hpp
#define CxConfigParser_hpp

#include <map>

class CxConfigParser {
private:
   std::map<String, String> configMap; // Stores the configuration key-value pairs
   String configString; // Holds the complete configuration string
   
   const char _cDelimiter = ';';
   const char _cAssigner = '=';
   
   void parseConfigString(const String &configStr) {
      configMap.clear(); // Clear any existing configuration
      configString = configStr;
      
      int start = 0;
      int end = configStr.indexOf(_cDelimiter);
      while (end != -1) {
         String entry = configStr.substring(start, end);
         entry.trim(); // Remove leading and trailing whitespace or special characters
         int separatorIndex = entry.indexOf(_cAssigner);
         if (separatorIndex != -1) {
            String key = entry.substring(0, separatorIndex);
            String value = entry.substring(separatorIndex + 1);
            key.trim(); // Trim key
            value.trim(); // Trim value
            configMap[key] = value;
         }
         start = end + 1;
         end = configStr.indexOf(_cDelimiter, start);
      }
   }

public:
   // Constructor
   CxConfigParser(const String &configStr) {
      parseConfigString(configStr);
   }
   
   CxConfigParser() {
      configMap.clear(); // Clear any existing configuration
   }
   
   // Method to get a string value by key
   String getStr(const String &key, const char* szDefault = "") {
      return configMap.count(key) ? configMap[key] : szDefault;
   }
   
   // Method to get a string value by key
   const char* getSz(const String &key, const char* szDefault = "") {
      return configMap.count(key) ? (configMap[key]).c_str() : szDefault;
   }

   
   // Method to get an integer value by key
   uint32_t getInt(const String &key, uint32_t nDefault = 0) {
      return configMap.count(key) ? (uint32_t)configMap[key].toInt() : nDefault;
   }
   
   // Method to get a float value by key
   float getFloat(const String &key, float fDefault = 0.0f) {
      return configMap.count(key) ? configMap[key].toFloat() : fDefault;
   }
   
   // Method to add a string variable
   void addVariable(const String strName, const String strValue) {
      configMap[strName] = strValue;
      rebuildConfigString();
   }
   
   // Method to add an integer variable
   void addVariable(const String strName, uint32_t nValue) {
      configMap[strName] = String(nValue);
      rebuildConfigString();
   }
   
   // avoid ambigious with float
   void addVariable(const String strName, uint16_t nValue) {
      configMap[strName] = String(nValue);
      rebuildConfigString();
   }
   
   // avoid ambigious with float
   void addVariable(const String strName, uint8_t nValue) {
      configMap[strName] = String(nValue);
      rebuildConfigString();
   }

   // Method to add a float variable
   void addVariable(const String strName, float fValue, uint8_t nPrecision = 2) {
      configMap[strName] = String(fValue, nPrecision); // Use 6 decimal places for float
      rebuildConfigString();
   }
   
   // Method to get the complete configuration string
   String& getConfigStr() {
      return configString;
   }
   
private:
   // Rebuild the configuration string from the map
   void rebuildConfigString() {
      configString = "";
      for (const auto &pair : configMap) {
         configString += pair.first + _cAssigner + pair.second + _cDelimiter;
      }
   }
};

// Example usage:
// CxConfigParser config("key1=val1;key2=123;key3=45.67;");
// Serial.println(config.getStr("key1")); // Output: val1
// Serial.println(config.getInt("key2")); // Output: 123
// Serial.println(config.getFloat("key3")); // Output: 45.67
// config.addVariable("key4", "newValue");
// config.addVariable("key5", 789);
// config.addVariable("key6", 3.14f);
// Serial.println(config.getConfigStr());


#endif /* CxConfigParser_hpp */


