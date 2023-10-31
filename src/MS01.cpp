#include <Arduino.h>
#include "MS01.h"
#include "../include/lightMath.h"

//Thanks Tasmota (https://tasmota.github.io/docs/)

MS01_t MS01;

MS01_t::MS01_t() {
    //this->pin = SENSOR_PIN;
    this->lastresult[0] = this->lastresult[1] = 0;
    this->wet_raw_value = WET_RAW;
}

float MS01_t::ConvertHumidity(float h, bool smoothing) {
    float result;
    if(smoothing){
        result = lastresult[1] * 0.33 + 0.33 * lastresult[0] + 0.33 * h;
    } else{
        result = h;
    }
    return result;
}

uint32_t MS01_t::ExpectPulse(bool level) {
    uint32_t count = 0;
    while (digitalRead(this->pin) == level) {
        if(count++ >= this->maxcycles){
            return UINT32_MAX;
        }
    }
    return count;
}

bool MS01_t::Read(){
    // Go into high impedence state to let pull-up raise data line level and
    // start the reading process.
    pinMode(this->pin, INPUT_PULLUP);
    delay(1);

    // First set data line low for a period
    pinMode(this->pin, OUTPUT);
    digitalWrite(this->pin, LOW);

    delayMicroseconds(this->delay_lo);

    uint32_t cycles[80] = { 0 };
    uint32_t i = 0;

    // End the start signal by setting data line high
    pinMode(this->pin, INPUT_PULLUP);
    delayMicroseconds(this->delay_hi);

    // Now start reading the data line to get the value from the sensor.

    // Turn off interrupts temporarily because the next sections
    // are timing critical and we don't want any interruptions.
    #ifdef ESP32
        {portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&mux);
    #else
        noInterrupts();
    #endif

    // First expect a low signal for ~80 microseconds followed by a high signal
    // for ~80 microseconds again.
    if ((ExpectPulse(LOW) != UINT32_MAX) && (ExpectPulse(HIGH) != UINT32_MAX)) {
    // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
    // microsecond low pulse followed by a variable length high pulse.  If the
    // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
    // then it's a 1.  We measure the cycle count of the initial 50us low pulse
    // and use that to compare to the cycle count of the high pulse to determine
    // if the bit is a 0 (high state cycle count < low state cycle count), or a
    // 1 (high state cycle count > low state cycle count). Note that for speed
    // all the pulses are read into a array and then examined in a later step.
        for (i = 0; i < 80; i += 2) {
            cycles[i] = ExpectPulse(LOW);
            if (cycles[i] == UINT32_MAX) { break; }
            cycles[i + 1] = ExpectPulse(HIGH);
            if (cycles[1 + i] == UINT32_MAX) { break; }
        }
    }
    #ifdef ESP32
        portEXIT_CRITICAL(&mux);}
    #else
        interrupts();
    #endif


    char cycle_dump[200] = { 0 };
    for (uint32_t i = 0; i < 20; i++) {
        snprintf_P(cycle_dump, sizeof(cycle_dump), PSTR("%s %u"), cycle_dump, cycles[i]);
    }

    if(i < 80){
        return false;
    }

    this->data[0] = this->data[1] = this->data[2] = this->data[3] = this->data[4] = 0;
    // Inspect pulses and determine which ones are 0 (high state cycle count < low
    // state cycle count), or 1 (high state cycle count > low state cycle count).
    for (int i = 0; i < 40; ++i) {
        uint32_t lowCycles = cycles[2 * i];
        uint32_t highCycles = cycles[2 * i + 1];
        this->data[i / 8] <<= 1;
        // Now compare the low and high cycle times to see if the bit is a 0 or 1.
        if (highCycles > lowCycles) {
            // High cycles are greater than 50us low cycle count, must be a 1.
            this->data[i / 8] |= 1;
        }
        // Else high cycles are less than (or equal to, a weird case) the 50us low
        // cycle count so this must be a zero.  Nothing needs to be changed in the
        // stored data.
    }

    uint8_t checksum = (this->data[0] + this->data[1] + this->data[2] + this->data[3]) & 0xFF;
    if (this->data[4] != checksum) {
        return false;
    }
    
    float temperature = NAN;
    float humidity = NAN;

    int16_t voltage = ((this->data[0] << 8) | this->data[1]);

    // Rough approximate of soil moisture % (based on values observed in the eWeLink app)
    // Observed values are available here: https://gist.github.com/minovap/654cdcd8bc37bb0d2ff338f8d144a509

    float x;
    if (voltage < 15037) {
        x = voltage - 15200;
        humidity = - FastPrecisePowf(0.0024f * x, 3) - 0.0004f * x + 20.1f;
    } else if (voltage < 22300) {
        humidity = - 0.00069f * voltage + 30.6f;
    } else {
        x = voltage - 22800;
        humidity = - FastPrecisePowf(0.00046f * x, 3) - 0.0004f * x + 15;
    }
    //temperature = 0;
    this->raw = voltage;

    if(isnan(humidity)) {
        return false;
    }

    if (humidity > 100) { humidity = 100.0f; }
    if (humidity < 0) { humidity = 0; }
    humidity = mapfloat(humidity, 0, this->wet_raw_value, 0, 100);
    if (humidity == 0) { humidity = 0.1f; }

    this->h = ConvertHumidity(humidity, true);
    this->lastresult[1] = this->lastresult[0];
    this->lastresult[0] = this->h;

    return true;
}