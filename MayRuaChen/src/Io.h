#pragma once
#include <stdint.h>

#include "Config.h"
#include "Controller.h"
#include "DebouncedInput.h"

// Ranh gioi duy nhat giua phan cung va logic:
//   poll()  - doc chan vao + xac nhan thoi gian (chong rung phao / nay nut)
//   read()  - gop thanh Inputs cho Controller
//   write() - dua Outputs ra 5 chan relay
//
// Doi chan, doi loai tiep diem NO/NC, doi relay active-HIGH/LOW: sua Config.h,
// khong dong vao file nay.
class Io {
public:
    void begin(uint32_t nowMs);
    void poll(uint32_t nowMs);            // goi moi 20 ms
    Inputs read(int16_t tempDeci);        // an su kien nut POWER
    void write(const Outputs& out) const;

private:
    DebouncedInput door_{config::kDoorClosedConfirmMs, config::kDoorOpenConfirmMs};
    DebouncedInput tank_{config::kFloatLowConfirmMs, config::kFloatFullConfirmMs};
    DebouncedInput boiler_{config::kFloatLowConfirmMs, config::kFloatFullConfirmMs};
    DebouncedInput power_{config::kButtonDebounceMs, config::kButtonDebounceMs};
    bool powerEvent_ = false;  // giu su kien nhan nut cho toi khi read() lay di
};
