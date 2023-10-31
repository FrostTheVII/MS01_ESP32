#ifndef lightMath_H
  #define lightMath_H
#endif

#include <Arduino.h>

float FastPrecisePowf(const float x, const float y);
double FastPrecisePow(double a, double b);

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);