#ifndef CX_PERSISTENT_IMPL_H
#define CX_PERSISTENT_IMPL_H

#define JSON_MAX_SIZE 1024 // Maximum size for JSON document

#ifdef ARDUINO
#include <FS.h>
#ifdef ESP32
#include "LITTLEFS.h"
struct FSInfo {
   size_t totalBytes;
   size_t usedBytes;
   size_t blockSize;
   size_t pageSize;
   size_t maxOpenFiles;
   size_t maxPathLength;
};
#define Dir File
#define LittleFS LITTLEFS
#else
#include <LittleFS.h>
#endif /* ESP32*/
#endif /* ARDUINO */

/// \class CxPersistentImpl
/// \brief A singleton class for managing persistent settings stored in a JSON file using LittleFS.
class CxPersistentImpl {
private:
   String _strFileName;
   
   /// \brief Private constructor to enforce the Singleton pattern.
   CxPersistentImpl() : _strFileName("/settings.json") {}
   
   /// \brief Loads the JSON document from the file.
   bool loadJson(DynamicJsonDocument& doc) {
#ifdef ARDUINO
      File file = LittleFS.open(_strFileName, "r");
      if (!file) {
         return false;
      }
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      return !error;
#else
      return false; // Not implemented for non-ARDUINO platforms
#endif
   }
   
   /// \brief Saves the JSON document to the file.
   bool saveJson(const DynamicJsonDocument& doc) {
#ifdef ARDUINO
      File file = LittleFS.open(_strFileName, "w");
      if (!file) {
         return false;
      }
      serializeJson(doc, file);
      file.close();
      return true;
#else
      return false; // Not implemented for non-ARDUINO platforms
#endif
   }
   
public:
   /// \brief Deleted copy constructor and assignment operator to prevent copying.
   CxPersistentImpl(const CxPersistentImpl&) = delete;
   CxPersistentImpl& operator=(const CxPersistentImpl&) = delete;
   
   /// \brief Provides access to the Singleton instance.
   static CxPersistentImpl& getInstance() {
      static CxPersistentImpl instance;
      return instance;
   }
   
   void setImplementation(CxPersistentBase& impl) {
      impl.setLoadStrFunc([this](const char* szName, const char* szDefaultValue, const char* szGroup) {
         return this->loadSettingStr(szName, szDefaultValue, szGroup);
      });
      impl.setLoadIntFunc([this](const char* szName, int32_t nDefaultValue, const char* szGroup) {
         return this->loadSettingInt(szName, nDefaultValue, szGroup);
      });
      impl.setSaveStrFunc([this](const char* szName, const char* szValue, const char* szComment, const char* szGroup) {
         return this->saveSettingStr(szName, szValue, szComment, szGroup);
      });
      impl.setSaveIntFunc([this](const char* szName, int32_t nValue, const char* szComment, const char* szGroup) {
         return this->saveSettingInt(szName, nValue, szComment, szGroup);
      });
   }
   
   /// \brief Sets the file name for storing settings.
   void setFileName(const char* szFileName) {
      _strFileName = szFileName;
   }
   
   /// \brief Saves a string setting to the JSON file.
   bool saveSettingStr(const char* szName, const char* szValue, const char* szComment = "", const char* szGroup = "") {
      DynamicJsonDocument doc(JSON_MAX_SIZE);
      if (!loadJson(doc)) {
         doc.clear(); // Start with an empty document if loading fails
      }
      JsonObject target;
      if (strlen(szGroup) > 0) {
         JsonObject group = doc[szGroup];
         if (!group) {
            group = doc.createNestedObject(szGroup);
         }
         target = group;
      } else {
         target = doc.as<JsonObject>();
      }
      
      target[szName] = szValue;
      
      return saveJson(doc);
   }
   
   
   /// \brief Saves an integer setting to the JSON file.
   bool saveSettingInt(const char* szName, int32_t nValue, const char* szComment = "", const char* szGroup = "") {
      // TODO: save as integer value, not as a string
      char szValue[12];
      
      snprintf(szValue, sizeof(szValue), "%d", nValue);
      return saveSettingStr(szName, szValue, szComment, szGroup);
   }
   
   /// \brief Saves a float setting to the JSON file.
   bool saveSettingFloat(const char* szName, float fValue, const char* szComment = "", const char* szGroup = "") {
      // TODO: save as float value, not as a string

      char szValue[16];
      
      snprintf(szValue, sizeof(szValue), "%.6f", fValue);
      return saveSettingStr(szName, szValue, szComment, szGroup);
   }
   
   /// \brief Saves a boolean setting to the JSON file.
   bool saveSettingBool(const char* szName, bool bValue, const char* szComment = "", const char* szGroup = "") {
      // TODO: save as bool value, not as a string
      return saveSettingStr(szName, bValue ? "true" : "false", szComment, szGroup);
   }
   
   /// \brief Loads a string setting from the JSON file.
   String loadSettingStr(const char* szName, const char* szDefaultValue, const char* szGroup = "") {
      DynamicJsonDocument doc(JSON_MAX_SIZE);
      if (!loadJson(doc)) {
         return String(szDefaultValue);
      }
      
      JsonObject group = strlen(szGroup) > 0 ? doc[szGroup].as<JsonObject>() : doc.as<JsonObject>();
      if (group && group.containsKey(szName)) {
         return String(group[szName].as<const char*>());
      }
      
      return String(szDefaultValue);
   }
   
   /// \brief Loads an integer setting from the JSON file.
   int32_t loadSettingInt(const char* szName, int32_t nDefaultValue, const char* szGroup = "") {
      String value = loadSettingStr(szName, String(nDefaultValue).c_str(), szGroup);
      return value.toInt();
   }
   
   /// \brief Loads a float setting from the JSON file.
   float loadSettingFloat(const char* szName, float fDefaultValue, const char* szGroup = "") {
      String value = loadSettingStr(szName, String(fDefaultValue, 6).c_str(), szGroup);
      return value.toFloat();
   }
   
   /// \brief Loads a boolean setting from the JSON file.
   bool loadSettingBool(const char* szName, bool bDefaultValue, const char* szGroup = "") {
      String value = loadSettingStr(szName, bDefaultValue ? "true" : "false", szGroup);
      return value.equalsIgnoreCase("true");
   }
};

#endif // CX_PERSISTENT_IMPL_H

