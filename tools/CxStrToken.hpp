//
//  CxToken.hpp
//  xESP
//
//  Created by ocfu on 09.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxToken_hpp
#define CxToken_hpp


// MARK: room for improvement here (code size)

#include <cstddef>
#include <cstdint>
#include <cstring>

#define MAX_TOKENS 8

#define TKTOCHAR(t,x)    ((t)[(x)].as<const char*>())
#define TKTOINT(t,x,y)   ((t)[(x)].as<int32_t>((y)))
#define TKTOFLOAT(t,x,y) ((t)[(x)].as<float>((y)))
#define TKTOCHARAFTER(t, x)    ((t).getStringAfter((x)))

class CxStrToken {
protected:
   char*        _szStrCopy;
   const char*  _szDelimiters;     // token delimiter as string
   char*        _aszTokens[MAX_TOKENS];    // Maximal n Tokens allowed
   uint8_t      _nCount;           // Count of found tokens
   
   mutable char* _result; // Mutable to allow modification in const method
   mutable uint8_t _currentIndex = 0;

private:

   void tokenize() {
      if (!_szStrCopy || !_szDelimiters) {
         return;
      }
      // TODO: allow escape from quotes with \"
      
      char* current = _szStrCopy;
      _nCount = 0;
      bool inQuotes = false;
      
      while (*current) {
         // Skip leading delimiters if not inside quotes
         if (!inQuotes) {
            current += strspn(current, _szDelimiters);
         }
         if (*current == '\0') {
            break; // No token left
         }
         
         // Check for the start of a quoted section
         if (*current == '\"') {
            inQuotes = !inQuotes; // Toggle quote state
            ++current; // Skip the quote character
         }
         
         // Store pointer to token
         _aszTokens[_nCount++] = current;
         
         // Find the end of the token
         while (*current && (inQuotes || !strchr(_szDelimiters, *current))) {
            if (*current == '\"') {
               inQuotes = !inQuotes; // Toggle quote state
            }
            ++current;
         }
         
         // Remove trailing quote if present
         if (*(current - 1) == '\"') {
            *(current - 1) = '\0';
         }
         
         // Replace delimiter with '\0' if not inside quotes
         if (*current != '\0') {
            *current++ = '\0';
         }
         
         // Stop if maximum tokens are reached
         if (_nCount == MAX_TOKENS) {
            break;
         }
      }
   }
public:
   class ctkProxy {
   private:
      const char* token;
      
   public:
      ctkProxy(const char* t) : token(t) {}
      
      template <typename T>
      T as(T defaultValue = T{}) const {
         if (!token) {
            return defaultValue; // Standardwert bei ungültigem Token
         }
         
         // Typspezialisierung durch SFINAE
         return asImpl<T>(defaultValue);
      }
      
      template <typename T>
      typename std::enable_if<std::is_same<T, const char*>::value, T>::type
      asImpl(T) const {
         return token; // Rückgabe als const char*
      }
      
      template <typename T>
      typename std::enable_if<std::is_same<T, int32_t>::value, T>::type
      asImpl(T defaultValue) const {
         char* end;
         int32_t value = (int32_t)std::strtol(token, &end, 0); // return as int32_t with auto base
         
         // Check if the conversion failed (no characters processed or out of range)
         if (end == token || *end != '\0') {
            return defaultValue; // Return the provided default value
         }
         return value;
      }
      
      template <typename T>
      typename std::enable_if<std::is_same<T, float>::value, T>::type
      asImpl(T defaultValue) const {
         char* end;
         float value = std::strtof(token, &end); // return als float
         
         // Check if the conversion failed (no characters processed or out of range)
         if (end == token || *end != '\0') {
            return defaultValue; // Return the provided default value
         }
         return value;
      }
   };

   CxStrToken() : _szStrCopy(nullptr), _szDelimiters(nullptr), _nCount(0), _result(nullptr) {reset();}
   CxStrToken(const char* sz, const char* szDelimiters)
   : CxStrToken() {
      setString(sz, szDelimiters);
   }
   
   ~CxStrToken() {
      delete [] _szStrCopy;
      delete [] _result;
   }
   
   // set the string to be tokenized
   void setString(const char* sz, const char* szDelimiters) {
      delete [] _szStrCopy;
      
      if (!sz || !szDelimiters) {
         // empty, nothing to do
         _szStrCopy = nullptr;
         _szDelimiters = nullptr;
         _nCount = 0;
         return;
      }

      // create a copy of the string as tokenize is destructive
      _szStrCopy = new char[std::strlen(sz)+1];
      std::strcpy(_szStrCopy, sz);
      _szDelimiters = szDelimiters;
      _nCount = 0;
      tokenize();
   }

   
   // returns the number of tokens
   uint8_t count() const {
      return _nCount;
   }
   
   // returns token at index i
   const char* item(uint8_t i) const {
      if (i >= _nCount) {
         return nullptr; // invalid index
      }
      return _aszTokens[i];
   }
/*
   const char* operator[](uint8_t i) const {
      return item(i); // access by operort []
   }
   
   int32_t toInt(uint8_t i, int32_t invalid = -1) const {
      if (i >= _nCount) {
         return invalid; // invalid index, returns given invalid number
      }
      return (int32_t) std::strtol(_aszTokens[i], nullptr, 10); // convert token to int32_t
   }
*/
   ctkProxy operator[](uint8_t i) const {
      if (i >= _nCount) {
         return ctkProxy(nullptr); // Ungültiger Index
      }
      return ctkProxy(_aszTokens[i]);
   }
   
   const char* getStringAfter(uint8_t startIndex) const {
      delete[] _result; // Delete previous result
      _result = nullptr;
      
      if (startIndex >= _nCount) {
         return ""; // Return empty string if startIndex is out of bounds
      }
      
      size_t totalLength = 0;
      for (uint8_t i = startIndex; i < _nCount; ++i) {
         totalLength += strlen(_aszTokens[i]) + 1; // +1 for space or null terminator
      }
      
      _result = new char[totalLength];
      _result[0] = '\0'; // Initialize as empty string
      
      for (uint8_t i = startIndex; i < _nCount; ++i) {
         strcat(_result, _aszTokens[i]);
         if (i < _nCount - 1) {
            strcat(_result, " ");
         }
      }
      
      return _result;
   }
   
   // Resets and returns the first token
   ctkProxy get() const {
      if (_nCount == 0 || _currentIndex >= _nCount) {
         return ctkProxy(nullptr);
      }
      return ctkProxy(_aszTokens[_currentIndex]);
   }
   
   // Advances to the next token and returns it
   ctkProxy next() const {
      if (_currentIndex + 1 < _nCount) {
         ++_currentIndex;
         return ctkProxy(_aszTokens[_currentIndex]);
      }
      // No more tokens, return invalid
      return ctkProxy(nullptr);
   }
   
   // Optionally, add a method to reset the index
   void reset() const {
      _currentIndex = 0;
   }
};

#define MAX_DELIMITERS 3

class CxMultiStrToken : public CxStrToken {
private:
   const char* _delimiters[MAX_DELIMITERS];
   uint8_t _delimiterCount;
   uint8_t _delimiterUsed[MAX_TOKENS];
   
   void tokenize() {
      if (!_szStrCopy || _delimiterCount == 0) return;
      char* current = _szStrCopy;
      _nCount = 0;
      bool inQuotes = false;
      while (*current && _nCount < MAX_TOKENS) {
         // Skip leading delimiters if not inside quotes
         if (!inQuotes) {
            bool skipped = false;
            for (uint8_t i = 0; i < _delimiterCount; ++i) {
               size_t len = strlen(_delimiters[i]);
               while (strncmp(current, _delimiters[i], len) == 0) {
                  current += len;
                  skipped = true;
               }
            }
            if (skipped && *current == '\0') break;
         }
         if (*current == '\0') break;
         
         // Handle quote start
         if (*current == '\"') {
            inQuotes = true;
            ++current;
         }
         
         _aszTokens[_nCount] = current;
         
         // Find the end of the token
         int foundDelimIdx = -1;
         char* foundDelimPos = nullptr;
         while (*current) {
            if (inQuotes) {
               if (*current == '\"') {
                  inQuotes = false;
                  ++current;
                  break;
               }
               ++current;
            } else {
               bool foundDelim = false;
               for (uint8_t i = 0; i < _delimiterCount; ++i) {
                  size_t len = strlen(_delimiters[i]);
                  if (strncmp(current, _delimiters[i], len) == 0) {
                     foundDelim = true;
                     foundDelimIdx = i;
                     foundDelimPos = current;
                     break;
                  }
               }
               if (foundDelim) break;
               if (*current == '\"') {
                  inQuotes = true;
               }
               ++current;
            }
         }
         
         if (foundDelimPos) {
            *foundDelimPos = '\0';
            _delimiterUsed[_nCount] = foundDelimIdx + 1; // 1-based index
            current = foundDelimPos + strlen(_delimiters[foundDelimIdx]);
         } else {
            _delimiterUsed[_nCount] = 0; // No delimiter after last token
         }
         ++_nCount;
      }
   }
   
public:
   CxMultiStrToken() : CxStrToken(), _delimiterCount(0) {}
   
   CxMultiStrToken(const char* sz, const char* delimiters[], uint8_t delimiterCount)
   : CxStrToken(), _delimiterCount(delimiterCount) {
      for (uint8_t i = 0; i < delimiterCount && i < MAX_DELIMITERS; ++i) {
         _delimiters[i] = delimiters[i];
      }
      setString(sz, delimiters, delimiterCount);
   }
   
   void setString(const char* sz, const char* delimiters[], uint8_t delimiterCount) {
      delete[] _szStrCopy;
      if (!sz || delimiterCount == 0) {
         _szStrCopy = nullptr;
         _nCount = 0;
         _delimiterCount = 0;
         return;
      }
      _szStrCopy = new char[strlen(sz) + 1];
      strcpy(_szStrCopy, sz);
      _delimiterCount = delimiterCount;
      for (uint8_t i = 0; i < delimiterCount && i < MAX_DELIMITERS; ++i) {
         _delimiters[i] = delimiters[i];
      }
      _nCount = 0;
      tokenize();
   }
   
   // Returns the 1-based delimiter index used for the token at i, or 0 if none
   uint8_t delimiterIndex(uint8_t i) const {
      if (i >= _nCount) return 0;
      return _delimiterUsed[i];
   }
};

#endif /* CxToken_hpp */
