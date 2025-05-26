//
//  espmath.h
//
//  Created by ocfu on 12.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef espmath_h
#define espmath_h

#include <cmath>

class ExprParser {
   const char* s;
   bool* bValid;
   
   float parseNumber() {
      float result = 0.0f;
      float sign = 1.0f;
      if (*s == '-') {
         sign = -1.0f;
         ++s;
      }
      if (!std::isdigit(*s) && *s != '.') {
         *bValid = false;
         return 0.0f;
      }
      while (std::isdigit(*s)) {
         result = result * 10 + (*s++ - '0');
      }
      if (*s == '.') {
         ++s;
         float frac = 1.0f;
         if (!std::isdigit(*s)) {
            *bValid = false;
            return 0.0f;
         }
         while (std::isdigit(*s)) {
            frac /= 10.0f;
            result += (*s++ - '0') * frac;
         }
      }
      return sign * result;
   }
   
   void skipSpaces() {
      while (std::isspace(*s)) ++s;
   }
   
   float parseFactor() {
      skipSpaces();
      float result = 0.0f;
      if (*s == '(') {
         ++s;
         result = parseExpr();
         if (*s == ')') ++s;
         else *bValid = false;
      } else {
         result = parseNumber();
      }
      skipSpaces();
      return result;
   }
   
   float parseTerm() {
      float result = parseFactor();
      while (*bValid) {
         skipSpaces();
         if (*s == '*') {
            ++s;
            result *= parseFactor();
         } else if (*s == '/') {
            ++s;
            float divisor = parseFactor();
            if (divisor == 0.0f) {
               *bValid = false;
               return 0.0f;
            }
            result /= divisor;
         } else {
            break;
         }
      }
      return result;
   }
   
   float parseExpr() {
      float result = parseTerm();
      while (*bValid) {
         skipSpaces();
         if (*s == '+') {
            ++s;
            result += parseTerm();
         } else if (*s == '-') {
            ++s;
            result -= parseTerm();
         } else {
            break;
         }
      }
      return result;
   }
   
public:
   float eval(const char* expr, bool& valid) {
      s = expr;
      bValid = &valid;
      valid = true;
      float result = parseExpr();
      skipSpaces();
      if (*s != '\0') valid = false;
      if (!valid) return 0.0f;
      return result;
   }
};

inline double roundToPrecision(double x, unsigned prec) {
   double factor = std::pow(10.0, prec);
   return std::round(x * factor) / factor;
}

#endif /* espmath_h */
