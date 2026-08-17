#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/src/Ui.h"
#pragma once
#include <stdint.h>

#include "Controller.h"
#include "LcdI2c.h"

// LCD 16x2 I2C. Chi ghi ra I2C dong nao co noi dung thay doi, khong bao gio
// goi clear() trong vong lap => khong nhap nhay.
//
// Neu khong tim thay LCD luc khoi dong thi may van chay binh thuong,
// chi la khong hien thi.
class Ui {
public:
    bool begin();
    void show(State st, const Inputs& in, uint16_t remainSec);
    bool available() const { return ok_; }

private:
    void setLine(uint8_t row, const char* text);

    LcdI2c lcd_;
    char shadow_[2][17] = {{'\0'}, {'\0'}};
    bool ok_ = false;
};
