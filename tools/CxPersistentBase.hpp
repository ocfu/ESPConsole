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
   using SaveStrFunc = std::function<bool(const char*, const char*, const char*, const char*)>;
   using SaveIntFunc = std::function<bool(const char*, int32_t, const char*, const char*)>;
   
private:
   // std::function members
   LoadStrFunc _loadStrFunc = nullptr;
   LoadIntFunc _loadIntFunc = nullptr;
   SaveStrFunc _saveStrFunc = nullptr;
   SaveIntFunc _saveIntFunc = nullptr;
   
public:
   /// \brief Sets the std::function for loading a string setting.
   void setLoadStrFunc(LoadStrFunc func) { _loadStrFunc = func; }
   
   /// \brief Sets the std::function for loading an integer setting.
   void setLoadIntFunc(LoadIntFunc func) { _loadIntFunc = func; }
      
   /// \brief Sets the std::function for saving a string setting.
   void setSaveStrFunc(SaveStrFunc func) { _saveStrFunc = func; }
   
   /// \brief Sets the std::function for saving an integer setting.
   void setSaveIntFunc(SaveIntFunc func) { _saveIntFunc = func; }
   
   /// \brief Loads a string setting using the std::function.
   String loadSettingStr(const char* szName, const char* szDefaultValue, const char* szGroup = "") {
      return _loadStrFunc ? _loadStrFunc(szName, szDefaultValue, szGroup) : String(szDefaultValue);
   }
   
   /// \brief Loads an integer setting using the std::function.
   int32_t loadSettingInt(const char* szName, int32_t nDefaultValue, const char* szGroup = "") {
      return _loadIntFunc ? _loadIntFunc(szName, nDefaultValue, szGroup) : nDefaultValue;
   }
      
   /// \brief Saves a string setting using the std::function.
   bool saveSettingStr(const char* szName, const char* szValue, const char* szComment = "", const char* szGroup = "") {
      return _saveStrFunc ? _saveStrFunc(szName, szValue, szComment, szGroup) : false;
   }
   
   /// \brief Saves an integer setting using the std::function.
   bool saveSettingInt(const char* szName, int32_t nValue, const char* szComment = "", const char* szGroup = "") {
      return _saveIntFunc ? _saveIntFunc(szName, nValue, szComment, szGroup) : false;
   }
      
};

#endif // CX_PERSISTENT_BASE_H
