#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/src/DebouncedInput.h"
#pragma once
#include <stdint.h>

// Debounce voi thoi gian xac nhan RIENG cho tung chieu.
//
// Vi du phao: can 3000 ms de cong nhan "thieu nuoc" (chong rung phao,
// requirement01 muc 8) nhung chi can 500 ms de cong nhan "du nuoc" (dong van
// kip, khong tran).
//
// Khong phu thuoc Arduino.h => test duoc tren PC.
class DebouncedInput {
public:
    constexpr DebouncedInput(uint16_t toActiveMs, uint16_t toInactiveMs)
        : toActiveMs_(toActiveMs), toInactiveMs_(toInactiveMs) {}

    // Nhan trang thai thuc te luc khoi dong, khong sinh su kien canh.
    void begin(bool active, uint32_t nowMs) {
        stable_ = active;
        candidate_ = active;
        sinceMs_ = nowMs;
        rose_ = false;
        fell_ = false;
    }

    // Goi moi chu ky quet (20 ms) voi gia tri doc tho tu chan.
    void update(bool raw, uint32_t nowMs) {
        rose_ = false;
        fell_ = false;

        if (raw != candidate_) {  // tin hieu vua doi -> bat dau dem lai tu dau
            candidate_ = raw;
            sinceMs_ = nowMs;
            return;
        }
        if (candidate_ == stable_) {
            return;
        }
        const uint16_t needMs = candidate_ ? toActiveMs_ : toInactiveMs_;
        // Phep tru unsigned: dung ca khi millis() tran sau 49.7 ngay.
        if (static_cast<uint32_t>(nowMs - sinceMs_) >= needMs) {
            stable_ = candidate_;
            rose_ = stable_;
            fell_ = !stable_;
        }
    }

    bool isActive() const { return stable_; }
    bool rose() const { return rose_; }  // vua chuyen sang ACTIVE (dung 1 lan)
    bool fell() const { return fell_; }

private:
    const uint16_t toActiveMs_;
    const uint16_t toInactiveMs_;
    bool stable_ = false;
    bool candidate_ = false;
    bool rose_ = false;
    bool fell_ = false;
    uint32_t sinceMs_ = 0;
};
