#ifndef CX_PERSISTENT_BASE_H
#define CX_PERSISTENT_BASE_H

#include <functional>

/// \class CxPersistentBase
/// \brief A base class for managing persistent settings using std::function.
class CxPersistentBase {
public:
   // Typedefs for std::function
   using LoadStrFunc = std::function<String(const char*, const char*, const char*)>;
   using LoadIntFunc = std::function<int32_t(const char*, int32_t, const char*)>;
   using LoadFloatFunc = std::function<float(const char*, float, const char*)>;
   using LoadBoolFunc = std::function<bool(const char*, bool, const char*)>;
   using SaveStrFunc = std::function<bool(const char*, const char*, const char*, const char*)>;
   using SaveIntFunc = std::function<bool(const char*, int32_t, const char*, const char*)>;
   using SaveFloatFunc = std::function<bool(const char*, float, const char*, const char*)>;
   using SaveBoolFunc = std::function<bool(const char*, bool, const char*, const char*)>;
   
private:
   // std::function members
   LoadStrFunc _loadStrFunc = nullptr;
   LoadIntFunc _loadIntFunc = nullptr;
   LoadFloatFunc _loadFloatFunc = nullptr;
   LoadBoolFunc _loadBoolFunc = nullptr;
   SaveStrFunc _saveStrFunc = nullptr;
   SaveIntFunc _saveIntFunc = nullptr;
   SaveFloatFunc _saveFloatFunc = nullptr;
   SaveBoolFunc _saveBoolFunc = nullptr;
   
public:
   /// \brief Sets the std::function for loading a string setting.
   void setLoadStrFunc(LoadStrFunc func) { _loadStrFunc = func; }
   
   /// \brief Sets the std::function for loading an integer setting.
   void setLoadIntFunc(LoadIntFunc func) { _loadIntFunc = func; }
   
   /// \brief Sets the std::function for loading a float setting.
   void setLoadFloatFunc(LoadFloatFunc func) { _loadFloatFunc = func; }
   
   /// \brief Sets the std::function for loading a boolean setting.
   void setLoadBoolFunc(LoadBoolFunc func) { _loadBoolFunc = func; }
   
   /// \brief Sets the std::function for saving a string setting.
   void setSaveStrFunc(SaveStrFunc func) { _saveStrFunc = func; }
   
   /// \brief Sets the std::function for saving an integer setting.
   void setSaveIntFunc(SaveIntFunc func) { _saveIntFunc = func; }
   
   /// \brief Sets the std::function for saving a float setting.
   void setSaveFloatFunc(SaveFloatFunc func) { _saveFloatFunc = func; }
   
   /// \brief Sets the std::function for saving a boolean setting.
   void setSaveBoolFunc(SaveBoolFunc func) { _saveBoolFunc = func; }
   
   /// \brief Loads a string setting using the std::function.
   String loadSettingStr(const char* szName, const char* szDefaultValue, const char* szGroup = "") {
      return _loadStrFunc ? _loadStrFunc(szName, szDefaultValue, szGroup) : String(szDefaultValue);
   }
   
   /// \brief Loads an integer setting using the std::function.
   int32_t loadSettingInt(const char* szName, int32_t nDefaultValue, const char* szGroup = "") {
      return _loadIntFunc ? _loadIntFunc(szName, nDefaultValue, szGroup) : nDefaultValue;
   }
   
   /// \brief Loads a float setting using the std::function.
   float loadSettingFloat(const char* szName, float fDefaultValue, const char* szGroup = "") {
      return _loadFloatFunc ? _loadFloatFunc(szName, fDefaultValue, szGroup) : fDefaultValue;
   }
   
   /// \brief Loads a boolean setting using the std::function.
   bool loadSettingBool(const char* szName, bool bDefaultValue, const char* szGroup = "") {
      return _loadBoolFunc ? _loadBoolFunc(szName, bDefaultValue, szGroup) : bDefaultValue;
   }
   
   /// \brief Saves a string setting using the std::function.
   bool saveSettingStr(const char* szName, const char* szValue, const char* szComment = "", const char* szGroup = "") {
      return _saveStrFunc ? _saveStrFunc(szName, szValue, szComment, szGroup) : false;
   }
   
   /// \brief Saves an integer setting using the std::function.
   bool saveSettingInt(const char* szName, int32_t nValue, const char* szComment = "", const char* szGroup = "") {
      return _saveIntFunc ? _saveIntFunc(szName, nValue, szComment, szGroup) : false;
   }
   
   /// \brief Saves a float setting using the std::function.
   bool saveSettingFloat(const char* szName, float fValue, const char* szComment = "", const char* szGroup = "") {
      return _saveFloatFunc ? _saveFloatFunc(szName, fValue, szComment, szGroup) : false;
   }
   
   /// \brief Saves a boolean setting using the std::function.
   bool saveSettingBool(const char* szName, bool bValue, const char* szComment = "", const char* szGroup = "") {
      return _saveBoolFunc ? _saveBoolFunc(szName, bValue, szComment, szGroup) : false;
   }
};

#endif // CX_PERSISTENT_BASE_H
