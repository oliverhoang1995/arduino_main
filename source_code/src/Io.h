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
    // Tham so la {thoi gian de len true, thoi gian de ve false}.
    // Phao: true = DU nuoc (xac nhan nhanh 500 ms de dong van kip, khong tran),
    //       false = THIEU nuoc (xac nhan 3000 ms de chong rung phao).
    DebouncedInput door_{config::kDoorClosedConfirmMs, config::kDoorOpenConfirmMs};
    DebouncedInput tank_{config::kFloatFullConfirmMs, config::kFloatLowConfirmMs};
    DebouncedInput boiler_{config::kFloatFullConfirmMs, config::kFloatLowConfirmMs};
    DebouncedInput power_{config::kButtonDebounceMs, config::kButtonDebounceMs};
    bool powerEvent_ = false;  // giu su kien nhan nut cho toi khi read() lay di
};
