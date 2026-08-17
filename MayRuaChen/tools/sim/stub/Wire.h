#pragma once
#include <stdint.h>

// Ban gia cua thu vien Wire, chi du de LcdI2c.cpp bien dich va chay tren PC.
// endTransmission() tra 0 = "co thiet bi tra loi" -> mo phong nhu la co LCD that,
// nho vay toan bo phan dinh dang chuoi trong Ui.cpp van duoc chay qua.
struct WireStub {
    void begin() {}
    void setClock(unsigned long) {}
    void beginTransmission(uint8_t) {}
    void write(uint8_t) {}
    uint8_t endTransmission() { return 0; }
};
static WireStub Wire;
