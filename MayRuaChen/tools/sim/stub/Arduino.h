#pragma once
#include <stdint.h>
#include <stdio.h>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
static const uint8_t A0 = 54;
// --- state cua mo phong, dung chung cho moi translation unit ---
inline uint32_t& msRef() { static uint32_t v = 0; return v; }
inline int& adcRef() { static int v = 512; return v; }
inline int* pinRef() { static int p[80]; return p; }          // muc logic tren chan vao
inline int* outRef() { static int o[80]; return o; }          // muc logic da ghi ra chan ra
inline void pinMode(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t p) { return pinRef()[p]; }
inline void digitalWrite(uint8_t p, uint8_t v) { outRef()[p] = v; }
inline int analogRead(uint8_t) { return adcRef(); }
inline uint32_t millis() { return msRef(); }
// Dong ho cua mo phong do tick() dieu khien, nen delay() la lenh rong.
// Chi LcdI2c dung tro nay, va chi trong luc khoi tao LCD.
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper*>(s))
struct SerialStub {
    void begin(long) {}
    void print(const __FlashStringHelper* s) { printf("%s", reinterpret_cast<const char*>(s)); }
    void print(const char* s) { printf("%s", s); }
    void print(char c) { printf("%c", c); }
    void print(unsigned char v) { printf("%u", v); }
    void print(int v) { printf("%d", v); }
    void print(unsigned int v) { printf("%u", v); }
    void print(long v) { printf("%ld", v); }
    void print(unsigned long v) { printf("%lu", v); }
    template <class T> void println(T v) { print(v); printf("\n"); }
};
static SerialStub Serial;
