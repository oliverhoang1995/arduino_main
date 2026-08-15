#pragma once
#include <stdint.h>

// Toan bo logic dieu khien cua may. KHONG include <Arduino.h>:
// nhan Inputs, tra Outputs => doi chan / doi board khong dong vao day,
// va chay duoc unit test tren PC.
//
// Tham chieu: plans_02/02-design.md muc 2 va muc 3.

enum class State : uint8_t {
    Standby,  // chua bam POWER
    Fill,     // dang nap nuoc, cho ca 2 bon day
    Heat,     // 2 bon day, dang ham toi 55 do C
    Ready,    // du 55 do C, cho cua dong
    Wash,     // bom rua 65 s
    Wait,     // nghi 5 s
    Rinse,    // bom trang 12 s
    Finish,   // xong 1 chu trinh, cho mo cua
};

// Tin hieu vao, DA duoc xac nhan thoi gian o tang Io.
struct Inputs {
    bool doorClosed = false;   // cua dang dong
    bool tankLow = false;      // Tank thieu nuoc (da xac nhan 3 s)
    bool boilerLow = false;    // Boiler thieu nuoc (da xac nhan 3 s)
    bool powerPressed = false; // su kien: vua bam nut POWER (chi true 1 lan)
    int16_t tempDeci = 0;      // nhiet do Boiler, 0.1 do C, hoac kTempInvalid
};

// Lenh cho 5 co cau chap hanh.
struct Outputs {
    bool washPump = false;
    bool rinsePump = false;
    bool heater = false;
    bool tankValve = false;
    bool boilerValve = false;
};

class Controller {
public:
    void begin(uint32_t nowMs);

    // Goi moi 20 ms.
    Outputs update(const Inputs& in, uint32_t nowMs);

    State state() const { return state_; }
    bool ready() const { return ready_; }  // nhiet do da du de vao chuong trinh rua

    // Thoi gian con lai cua buoc hien tai, lam tron len (cho LCD).
    // Tra 0 neu state hien tai khong phai Wash/Wait/Rinse.
    uint16_t stepRemainingSec(uint32_t nowMs) const;

private:
    void enter(State next, uint32_t nowMs);
    void updateReady(int16_t tempDeci);
    bool updateHeater(const Inputs& in, uint32_t nowMs, bool enabled);
    uint32_t stepDurationMs() const;

    State state_ = State::Standby;
    uint32_t stepStartMs_ = 0;
    bool ready_ = false;
    bool heaterOn_ = false;
    uint32_t heaterChangedMs_ = 0;
};
