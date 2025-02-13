//
//  espmath.h
//
//  Created by ocfu on 12.12.24.
//  Copyright © 2024 ocfu. All rights reserved.
//

#ifndef espmath_h
#define espmath_h

#include <cmath>

inline double roundToPrecision(double x, unsigned prec) {
   double factor = std::pow(10.0, prec);
   return std::round(x * factor) / factor;
}

#endif /* espmath_h */
