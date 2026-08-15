#pragma once
#include <stdint.h>

#include "Config.h"

// Doc NTC 10K -> nhiet do don vi 0.1 do C.
//
// Chuoi xu ly: analogRead -> median 5 mau -> loc EMA -> cong thuc beta.
// Median la bat buoc vi trong tu dien co contactor thanh nhiet dong cat,
// xung nhieu vao ADC la chac chan co.
//
// Tra config::kTempInvalid khi ADC nam ngoai dai hop le (dut day / ngan mach) -
// khi do Controller se khong cho thanh nhiet chay.
class Ntc {
public:
    void begin();
    void sample();  // goi moi 100 ms
    int16_t deci() const { return deci_; }

private:
    uint16_t samples_[config::kNtcMedianSamples] = {0};
    uint8_t index_ = 0;
    bool filled_ = false;
    bool emaReady_ = false;
    int32_t ema16_ = 0;  // nhiet do * 16, giu do phan giai khi loc bang so nguyen
    int16_t deci_ = config::kTempInvalid;
};
