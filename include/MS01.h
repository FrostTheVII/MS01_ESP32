#ifndef MS01_H
#define MS01_H
#endif

#define WET_RAW 50; // Value observed with Sensor immersed in water

#include <Arduino.h>

class MS01_t
{
public:
  uint16_t delay_lo = 450; // Tasmota says 400 but my ESP32 only works with 450, I tested also in a ESP8266 and 450 works just fine
  uint16_t delay_hi = 30;  // Doesn't matter the board, always 30
  int16_t raw;
  char stype[12];
  int8_t pin;
  uint8_t lastresult[2];
  uint32_t maxcycles = microsecondsToClockCycles(1000);
  uint8_t data[5];
  float h;
  uint16_t wet_raw_value;

  MS01_t();

  float ConvertHumidity(float h, bool smoothing);
  bool Read();
  uint32_t ExpectPulse(bool level);
};

extern MS01_t MS01;
extern MS01_t MS01_1;
extern MS01_t MS01_2;
extern MS01_t MS01_3;