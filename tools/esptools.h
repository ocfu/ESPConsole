// esptools.h
// External ESPConsole tools header file

#ifndef esptools_h
#define esptools_h

#include <ctype.h>  // for isxdigit
#include <Arduino.h> // for String class

///
/// @brief Converts a single hex digit character to its integer value (0-15).
/// 
/// @param c Hex digit character ('0'-'9', 'a'-'f', 'A'-'F')
/// @return Integer value of the hex digit, or -1 if the character is not a valid hex digit
///
int hexDigitValue(char c) {
   if (c >= '0' && c <= '9') return c - '0';
   if (c >= 'a' && c <= 'f') return c - 'a' + 10;
   if (c >= 'A' && c <= 'F') return c - 'A' + 10;
   return -1;
}

///
/// @brief Converts all "0xXX" sequences in the input string to their corresponding characters.
/// - "0x41" becomes 'A'
/// - "0x0A" becomes '\n' (newline)
/// - "0x00" remains unchanged as the text "0x00" (no null byte!)
/// - Invalid sequences (e.g., "0xG1", "0x1", "0x123") are not replaced.
///
/// @param sz Input C-string (null-terminated)
/// @param strTarget Target string that will be overwritten with the converted result  
///
void convertHexSequences(const char* sz, String& strTarget) {
   strTarget = "";  
   if (sz == nullptr) return;

   size_t len = strlen(sz);
   for (size_t i = 0; i < len;) {
      // Check for "0x" followed by two hex digits
      if (sz[i] == '0' && i + 1 < len && sz[i + 1] == 'x' && i + 3 < len) {
         int hi = hexDigitValue(sz[i + 2]);
         int lo = hexDigitValue(sz[i + 3]);
         if (hi >= 0 && lo >= 0) {       // both characters are hex digits
            int value = (hi << 4) | lo;  // 0..255
            if (value != 0) {            // do not convert 0x00
               strTarget += char(value);
               i += 4;  // skip 4 characters
               continue;
            }
         }
      }
      // Not a valid "0xXX" or value == 0 -> take normal character
      strTarget += sz[i];
      i++;
   }
}

#endif /* esptools_h */