//
//  CxToken.hpp
//  xESP
//
//  Created by ocfu on 09.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef CxToken_hpp
#define CxToken_hpp


#include <cstddef>
#include <cstdint>
#include <cstring>

#define MAX_TOKENS 64

#define TKTOCHAR(t,x)    ((t)[(x)].as<const char*>())
#define TKTOINT(t,x,y)   ((t)[(x)].as<int32_t>((y)))
#define TKTOFLOAT(t,x,y) ((t)[(x)].as<float>((y)))

class CxStrToken {
private:
   char*        _szStrCopy;
   const char*  _szDelimiters;     // token delimiter as string
   char*        _aszTokens[MAX_TOKENS];    // Maximal n Tokens allowed
   uint8_t      _nCount;           // Count of found tokens
   
   void tokenize() {
      if (!_szStrCopy || !_szDelimiters) {
         return;
      }
      
      char* current = _szStrCopy;
      _nCount = 0;
      
      while (*current) {
         // skip heading delimiters
         current += strspn(current, _szDelimiters);
         if (*current == '\0') {
            break; // no token left
         }
         
         // store pointer to token
         _aszTokens[_nCount++] = current;
         
         // find the end of the token
         current += strcspn(current, _szDelimiters);
         
         // replace delimiter with '\0'
         if (*current != '\0') {
            *current++ = '\0';
         }
         
         // Maximal 255 Tokens speichern
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
      asImpl(T) const {
         return (int32_t)std::strtol(token, nullptr, 10); // Rückgabe als int32_t
      }
      
      template <typename T>
      typename std::enable_if<std::is_same<T, float>::value, T>::type
      asImpl(T) const {
         return std::strtof(token, nullptr); // Rückgabe als float
      }
   };

   CxStrToken() : _szStrCopy(nullptr), _szDelimiters(nullptr), _nCount(0) {}
   CxStrToken(const char* sz, const char* szDelimiters)
   : CxStrToken() {
      setString(sz, szDelimiters);
   }
   
   ~CxStrToken() {
      delete [] _szStrCopy;
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

};

#endif /* CxToken_hpp */
