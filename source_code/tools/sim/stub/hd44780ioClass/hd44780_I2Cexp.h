#pragma once
#include <stdint.h>
class hd44780_I2Cexp {
public:
    int begin(int, int) { return 0; }
    void clear() {}
    void setCursor(int, int) {}
    void print(const char*) {}
};
