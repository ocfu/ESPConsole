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

/**
 * Applies robust smoothing with absolute outlier rejection.
 *
 * @param reference   The previous known good value.
 * @param value       The new input value.
 * @param maxDiff     Maximum absolute difference allowed (outlier threshold).
 * @param threshold   (Optional) Difference scale for alpha ramp. 0 = fixed minAlpha. Use INVALID_FLOAT to skip smoothing.
 * @param minAlpha    (Optional) Minimum smoothing factor (0.0–1.0). Required if threshold is set.
 * @param maxAlpha    (Optional) Maximum smoothing factor (0.0–1.0). Required if threshold is set.
 *
 * @return Smoothed value or previous value if rejected as outlier.
 */
float smoothRobust(float reference, float value, float maxDiff,
                   float threshold = INVALID_FLOAT, float minAlpha = INVALID_FLOAT, float maxAlpha = INVALID_FLOAT) {
   // Step 0: first call scenario and validate values
   if (std::isnan(reference) || std::isnan(value) || std::isnan(maxDiff)) {
      return value;
   }
   
   float diff = fabsf(value - reference);
   
   // Step 1: Outlier rejection
   if (diff > maxDiff) {
      return reference;
   }
   
   // Step 2: No smoothing if parameters are missing
   if (std::isnan(threshold) || std::isnan(minAlpha) || std::isnan(maxAlpha)) {
      return value;
   }
   
   // Step 3: Apply smoothing
   float alpha;
   if (threshold <= 0.0f) {
      // Fixed alpha
      alpha = minAlpha;
   } else {
      // Adaptive alpha
      float scaled = diff / threshold;
      if (scaled > 1.0f) scaled = 1.0f;
      alpha = minAlpha + (maxAlpha - minAlpha) * scaled;
   }
   
   return alpha * value + (1.0f - alpha) * reference;
}



#endif /* espmath_h */
