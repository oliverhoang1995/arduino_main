#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/src/Ntc.cpp"
#include "Ntc.h"

#include <Arduino.h>
#include <math.h>

namespace {

constexpr uint8_t kN = config::kNtcMedianSamples;

uint16_t medianOf(const uint16_t* src) {
    uint16_t a[kN];
    for (uint8_t i = 0; i < kN; ++i) {
        a[i] = src[i];
    }
    // Insertion sort - kN = 5, khong can gi phuc tap hon.
    for (uint8_t i = 1; i < kN; ++i) {
        const uint16_t key = a[i];
        int8_t j = static_cast<int8_t>(i) - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = key;
    }
    return a[kN / 2];
}

// Cong thuc beta. Mach: 5V -- Rs -- A0 -- NTC -- GND
//   Rntc = Rs * adc / (1023 - adc)
//   1/T  = 1/298.15 + ln(Rntc/R0) / beta
int16_t adcToDeci(uint16_t adc) {
    if (adc >= config::kAdcOpenThreshold || adc <= config::kAdcShortThreshold) {
        return config::kTempInvalid;
    }
    const float rNtc = config::kNtcSeriesR * static_cast<float>(adc) / (1023.0f - static_cast<float>(adc));
    const float invT = 1.0f / 298.15f + logf(rNtc / config::kNtcR0) / config::kNtcBeta;
    const float tempC = 1.0f / invT - 273.15f;
    if (tempC < -50.0f) {
        return -500;
    }
    if (tempC > 150.0f) {
        return 1500;
    }
    return static_cast<int16_t>(lroundf(tempC * 10.0f));
}

}  // namespace

void Ntc::begin() {
    pinMode(config::kNtcPin, INPUT);
    analogRead(config::kNtcPin);  // bo mau dau tien sau khi doi kenh ADC
    for (uint8_t i = 0; i < kN; ++i) {
        samples_[i] = static_cast<uint16_t>(analogRead(config::kNtcPin));
    }
    filled_ = true;
    index_ = 0;
    sample();
}

void Ntc::sample() {
    samples_[index_] = static_cast<uint16_t>(analogRead(config::kNtcPin));
    ++index_;
    if (index_ >= kN) {
        index_ = 0;
        filled_ = true;
    }
    if (!filled_) {
        return;  // chua du mau de lay median
    }

    const int16_t raw = adcToDeci(medianOf(samples_));
    if (raw == config::kTempInvalid) {
        emaReady_ = false;
        deci_ = config::kTempInvalid;
        return;
    }

    const int32_t x16 = static_cast<int32_t>(raw) * 16;
    if (!emaReady_) {
        ema16_ = x16;
        emaReady_ = true;
    } else {
        ema16_ += (x16 - ema16_) / 5;  // EMA alpha = 0.2
    }
    deci_ = static_cast<int16_t>(ema16_ / 16);
}
