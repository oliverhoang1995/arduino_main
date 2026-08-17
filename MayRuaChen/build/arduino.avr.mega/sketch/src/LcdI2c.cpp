#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/src/LcdI2c.cpp"
#include "LcdI2c.h"

#include <Arduino.h>
#include <Wire.h>

#include "Config.h"

// So do chan cua module PCF8574 -> HD44780 (kieu pho bien nhat tren thi truong):
//
//   P0 -> RS   P1 -> RW   P2 -> EN   P3 -> den nen
//   P4 -> D4   P5 -> D5   P6 -> D6   P7 -> D7
//
// Neu module cua ban noi khac (LCD sang den nhung khong ra chu) thi sua 4 hang
// so duoi day, khong dong vao phan con lai.
namespace {

constexpr uint8_t kBitRs = 0x01;
// P1 (RW) luon giu muc 0: chi ghi, khong bao gio doc busy flag.
constexpr uint8_t kBitEn = 0x04;
constexpr uint8_t kBitBacklight = 0x08;

// Lenh HD44780
constexpr uint8_t kCmdClear = 0x01;
constexpr uint8_t kCmdEntryMode = 0x06;      // tang con tro, khong dich man hinh
constexpr uint8_t kCmdDisplayOff = 0x08;
constexpr uint8_t kCmdDisplayOn = 0x0C;      // bat hien thi, tat con tro, tat nhap nhay
constexpr uint8_t kCmdFunctionSet = 0x28;    // 4-bit, 2 dong, font 5x8
constexpr uint8_t kCmdSetDdram = 0x80;

constexpr uint8_t kRowOffset[2] = {0x00, 0x40};

// Thoi gian thuc thi cua HD44780. Khong doc busy flag (RW noi dat) nen phai cho
// du theo datasheet.
constexpr uint16_t kExecUs = 50;        // lenh thuong: 37 us
constexpr uint8_t kExecClearMs = 2;     // clear / return home: 1.52 ms

}  // namespace

// Mot nibble = mot xung EN. Gop ca 3 buoc (EN thap - EN cao - EN thap) vao
// cung mot goi I2C: it giao dich hon, va moi byte tren bus da mat >= 22 us nen
// do rong xung EN thua suc yeu cau 450 ns cua HD44780.
void LcdI2c::writeNibble(uint8_t nibble, uint8_t rs) {
    const uint8_t data = static_cast<uint8_t>((nibble & 0x0F) << 4) | kBitBacklight | rs;

    Wire.beginTransmission(addr_);
    Wire.write(data);
    Wire.write(static_cast<uint8_t>(data | kBitEn));
    Wire.write(data);
    Wire.endTransmission();

    delayMicroseconds(kExecUs);
}

void LcdI2c::writeByte(uint8_t value, uint8_t rs) {
    writeNibble(static_cast<uint8_t>(value >> 4), rs);
    writeNibble(static_cast<uint8_t>(value & 0x0F), rs);
}

void LcdI2c::command(uint8_t value) { writeByte(value, 0); }

bool LcdI2c::begin() {
    Wire.begin();
    Wire.setClock(config::kI2cClockHz);

    addr_ = 0;
    for (uint8_t i = 0; i < config::kLcdAddressCount; ++i) {
        Wire.beginTransmission(config::kLcdAddresses[i]);
        if (Wire.endTransmission() == 0) {
            addr_ = config::kLcdAddresses[i];
            break;
        }
    }
    if (addr_ == 0) {
        return false;
    }

    // HD44780 can >= 40 ms sau khi len nguon moi nhan lenh.
    delay(50);

    // Chuoi dua chip ve 4-bit theo datasheet: ba lan 0x3 (8-bit), roi 0x2.
    // Bat buoc phai lam du - chip co the dang o 8-bit hoac dang do dang 4-bit
    // neu MCU vua reset giua chung.
    writeNibble(0x03, 0);
    delay(5);
    writeNibble(0x03, 0);
    delayMicroseconds(150);
    writeNibble(0x03, 0);
    delayMicroseconds(150);
    writeNibble(0x02, 0);
    delayMicroseconds(150);

    command(kCmdFunctionSet);
    command(kCmdDisplayOff);
    command(kCmdClear);
    delay(kExecClearMs);
    command(kCmdEntryMode);
    command(kCmdDisplayOn);
    return true;
}

void LcdI2c::clear() {
    if (addr_ == 0) {
        return;
    }
    command(kCmdClear);
    delay(kExecClearMs);
}

void LcdI2c::setCursor(uint8_t col, uint8_t row) {
    if (addr_ == 0 || row > 1) {
        return;
    }
    command(static_cast<uint8_t>(kCmdSetDdram | (kRowOffset[row] + col)));
}

void LcdI2c::print(const char* text) {
    if (addr_ == 0) {
        return;
    }
    for (const char* p = text; *p != '\0'; ++p) {
        writeByte(static_cast<uint8_t>(*p), kBitRs);
    }
}
