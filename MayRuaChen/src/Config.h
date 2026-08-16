#pragma once
#include <stdint.h>

// Toan bo pin mapping va tham so cua may nam trong file nay.
// Doi phan cung hoac doi tham so => chi sua o day, khong dong vao logic.
//
// LUU Y: file nay KHONG duoc include <Arduino.h> vi Controller.cpp dung no
// va Controller phai bien dich duoc tren PC de chay unit test.

namespace config {

// ---------------------------------------------------------------- chan vao
// Tiep diem noi ve GND, dung INPUT_PULLUP.
// invert = true neu tiep diem la NC (dong khi KHONG co su kien).
struct InputPin {
    uint8_t pin;
    bool invert;
};

constexpr InputPin kDoor{22, false};         // ACTIVE = cua DONG
constexpr InputPin kTankFloat{23, false};    // ACTIVE = Tank THIEU nuoc
constexpr InputPin kBoilerFloat{24, false};  // ACTIVE = Boiler THIEU nuoc
constexpr InputPin kBtnPower{26, false};     // ACTIVE = dang nhan

// D25 (MODE), D27 (UP), D28 (DOWN): chua dung - xem plans_02/02-design.md Q-02.

constexpr uint8_t kNtcPin = 54;  // = A0 tren Mega 2560 (Io.cpp co static_assert kiem tra)

// ----------------------------------------------------------------- chan ra
// activeLow = true voi module relay pho bien (keo LOW thi relay dong).
struct OutputPin {
    uint8_t pin;
    bool activeLow;
};

constexpr OutputPin kWashPump{2, true};
constexpr OutputPin kRinsePump{3, true};
constexpr OutputPin kHeater{4, true};
constexpr OutputPin kTankValve{5, true};
constexpr OutputPin kBoilerValve{6, true};

// D7, D8 (bom hoa chat rua / trang): dieu khien ngoai Arduino - xem Q-01.

// -------------------------------------------------------- thoi gian chu trinh
constexpr uint32_t kWashMs = 65000;   // requirement01 muc 6 step 1
constexpr uint32_t kWaitMs = 5000;    // requirement01 muc 6 step 2
constexpr uint32_t kRinseMs = 12000;  // requirement01 muc 6 step 3

// ------------------------------------------------- nhiet do (don vi 0.1 do C)
constexpr int16_t kTempInvalid = -32768;  // khong doc duoc NTC (ho / ngan mach)

constexpr int16_t kSetpointDeci = 850;    // 85.0 do C - nhiet do dich cua Boiler
constexpr int16_t kHeaterHystDeci = 30;   // vung chet 3.0 do C
constexpr int16_t kReadyDeci = 550;       // 55.0 do C - nguong cho phep vao chuong trinh rua
constexpr int16_t kReadyHystDeci = 10;    // 1.0 do C - chong nhay qua lai quanh nguong

// ----------------------------------------------------- xac nhan tin hieu (ms)
constexpr uint16_t kFloatLowConfirmMs = 3000;    // requirement01 muc 8: chong rung phao
constexpr uint16_t kFloatFullConfirmMs = 500;    // dong van nhanh de khong tran
constexpr uint16_t kDoorClosedConfirmMs = 100;
constexpr uint16_t kDoorOpenConfirmMs = 50;      // mo cua phai nhan ra nhanh (tat bom)
constexpr uint16_t kButtonDebounceMs = 30;

// ------------------------------------------------------- bao ve contactor (ms)
constexpr uint32_t kHeaterMinOnMs = 5000;
constexpr uint32_t kHeaterMinOffMs = 5000;

// -------------------------------------------------------------------- NTC 10K
// Mach: 5V -- Rs 10k -- A0 -- NTC -- GND
constexpr float kNtcBeta = 3950.0f;
constexpr float kNtcR0 = 10000.0f;       // dien tro NTC tai 25 do C
constexpr float kNtcSeriesR = 10000.0f;  // dien tro noi tiep
constexpr uint16_t kAdcOpenThreshold = 1015;  // >= : dut day / ho mach
constexpr uint16_t kAdcShortThreshold = 8;    // <= : ngan mach
constexpr uint8_t kNtcMedianSamples = 5;

// -------------------------------------------------------------- LCD 16x2 I2C
// Module "LCD backpack" chay chip PCF8574. Hai dia chi pho bien:
//   0x27 - PCF8574    0x3F - PCF8574A
// Ui::begin() do lan luot tren bus, dung dia chi nao tra loi truoc.
// Neu module cua ban dat dia chi khac (han jumper A0/A1/A2): sua danh sach nay.
constexpr uint8_t kLcdAddresses[] = {0x27, 0x3F};
constexpr uint8_t kLcdAddressCount = sizeof(kLcdAddresses) / sizeof(kLcdAddresses[0]);

// Ha xuong 100000 neu cap I2C di dai trong tu dien hoac LCD hien ky tu rac.
constexpr uint32_t kI2cClockHz = 400000;

// ------------------------------------------------------------ chu ky task (ms)
constexpr uint16_t kInputPeriodMs = 20;
constexpr uint16_t kNtcPeriodMs = 100;
constexpr uint16_t kControlPeriodMs = 20;
constexpr uint16_t kUiPeriodMs = 250;

}  // namespace config
