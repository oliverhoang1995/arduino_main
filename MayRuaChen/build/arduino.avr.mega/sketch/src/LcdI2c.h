#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/src/LcdI2c.h"
#pragma once
#include <stdint.h>

// Driver LCD 16x2 HD44780 gan tren module I2C PCF8574 ("LCD backpack").
//
// CHI dung <Wire.h> - thu vien co san trong Arduino IDE. Khong can cai
// them thu vien nao qua Library Manager.
//
// Dia chi I2C + toc do bus: xem Config.h.
// So do chan PCF8574 -> HD44780: xem dau file LcdI2c.cpp.
class LcdI2c {
public:
    // Do tim LCD tren bus I2C roi khoi tao che do 4-bit, 2 dong, tat con tro.
    // Tra false neu khong module nao tra loi - khi do may van chay binh thuong,
    // chi la khong hien thi.
    bool begin();

    void clear();
    void setCursor(uint8_t col, uint8_t row);
    void print(const char* text);

    uint8_t address() const { return addr_; }  // 0 neu chua tim thay

private:
    void writeNibble(uint8_t nibble, uint8_t rs);
    void writeByte(uint8_t value, uint8_t rs);
    void command(uint8_t value);

    uint8_t addr_ = 0;
};
