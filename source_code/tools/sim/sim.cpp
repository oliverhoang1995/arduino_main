// Mo phong end-to-end: goi truc tiep setup()/loop() cua main.cpp voi chan + dong ho gia.
#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "Config.h"

void setup();
void loop();

// Tiep diem dong = LOW (INPUT_PULLUP)
static void contact(uint8_t pin, bool closed) { pinRef()[pin] = closed ? LOW : HIGH; }

static bool out(const config::OutputPin& c) {
    const int level = outRef()[c.pin];
    return c.activeLow ? (level == LOW) : (level == HIGH);
}

// nhiet do (do C) -> gia tri ADC, dung mach 5V--Rs--A0--NTC--GND
static int adcForTemp(double tC) {
    const double t = tC + 273.15;
    const double r = 10000.0 * exp(3950.0 * (1.0 / t - 1.0 / 298.15));
    return (int)(1023.0 * r / (10000.0 + r) + 0.5);
}

static double g_temp = 20.0;
static bool g_heating = false;

static void tick(uint32_t ms) {
    for (uint32_t i = 0; i < ms; ++i) {
        msRef() += 1;
        // mo phong nhiet: thanh nhiet bat -> tang 1 do C/s, tat -> nguoi 0.05 do C/s
        g_heating = out(config::kHeater);
        g_temp += (g_heating ? 1.0 : -0.05) / 1000.0;
        adcRef() = adcForTemp(g_temp);
        loop();
    }
}

static void banner(const char* s) { printf("\n---- t=%lus  %s ----\n", (unsigned long)(msRef() / 1000), s); }

static void dump(const char* tag) {
    printf("[%6lus] %-22s wash=%d rinse=%d heat=%d vTank=%d vBoil=%d  T=%.1f\n",
           (unsigned long)(msRef() / 1000), tag, out(config::kWashPump), out(config::kRinsePump),
           out(config::kHeater), out(config::kTankValve), out(config::kBoilerValve), g_temp);
}

static uint32_t waitUntil(bool (*pred)(), uint32_t limitMs) {
    for (uint32_t i = 0; i < limitMs; ++i) {
        msRef() += 1;
        g_heating = out(config::kHeater);
        g_temp += (g_heating ? 1.0 : -0.05) / 1000.0;
        adcRef() = adcForTemp(g_temp);
        loop();
        if (pred()) {
            return i + 1;
        }
    }
    return limitMs;
}
static bool washOn() { return out(config::kWashPump); }
static bool washOff() { return !out(config::kWashPump); }
static bool rinseOn() { return out(config::kRinsePump); }
static bool rinseOff() { return !out(config::kRinsePump); }

static void pressPower() {
    contact(config::kBtnPower.pin, true);
    tick(120);
    contact(config::kBtnPower.pin, false);
    tick(60);
}

int main() {
    for (int i = 0; i < 80; ++i) {
        pinRef()[i] = HIGH;
    }
    // Trang thai ban dau: cua mo, ca 2 bon can, nuoc 20 do C
    contact(config::kDoor.pin, false);
    contact(config::kTankFloat.pin, true);
    contact(config::kBoilerFloat.pin, true);
    adcRef() = adcForTemp(g_temp);

    setup();
    dump("sau setup (STANDBY)");

    banner("bam POWER");
    pressPower();
    dump("sau POWER");

    banner("Boiler day o t=20s");
    tick(19000);
    contact(config::kBoilerFloat.pin, false);
    tick(400);
    dump("sau 0.4s (chua du 0.5s)");
    tick(200);
    dump("sau 0.6s -> van Boiler dong");

    banner("Tank day o t=40s");
    tick(20000);
    contact(config::kTankFloat.pin, false);
    tick(600);
    dump("2 bon day -> gia nhiet");

    banner("dun toi 55 do C, cua VAN MO");
    tick(40000);
    dump("dang dun");
    while (g_temp < 86.0 && msRef() < 200000) {
        tick(1000);
    }
    dump("toi 85 do C, cua mo -> khong rua");
    tick(20000);
    dump("giu quanh 85 do C");

    banner("dong cua -> chu trinh rua (do chinh xac tung buoc)");
    contact(config::kDoor.pin, true);
    const uint32_t tStart = waitUntil(washOn, 3000);
    const uint32_t washMs = waitUntil(washOff, 80000);
    const uint32_t waitMs = waitUntil(rinseOn, 20000);
    const uint32_t rinseMs = waitUntil(rinseOff, 30000);
    printf("  dong cua -> bat bom rua : %5lu ms\n", (unsigned long)tStart);
    printf("  WASH                    : %5lu ms  (mong doi 65000)\n", (unsigned long)washMs);
    printf("  WAIT                    : %5lu ms  (mong doi  5000)\n", (unsigned long)waitMs);
    printf("  RINSE                   : %5lu ms  (mong doi 12000)\n", (unsigned long)rinseMs);
    dump("FINISH");

    banner("phao Tank rung 1s trong luc dang cho -> KHONG duoc mo van");
    contact(config::kTankFloat.pin, true);
    tick(1000);
    contact(config::kTankFloat.pin, false);
    tick(1000);
    dump("sau khi rung phao");

    banner("mo cua lay do -> READY, dong cua -> chu trinh moi");
    contact(config::kDoor.pin, false);
    tick(500);
    dump("cua mo -> READY");
    contact(config::kDoor.pin, true);
    tick(500);
    dump("cua dong -> WASH chu trinh 2");

    banner("thieu nuoc Tank giu 3s trong luc dang RUA");
    contact(config::kTankFloat.pin, true);
    tick(2900);
    dump("moi 2.9s -> van chua mo, bom van chay");
    tick(200);
    dump("du 3s -> van mo, bom VAN chay");
    contact(config::kTankFloat.pin, false);
    tick(700);
    dump("day lai -> van dong, bom van chay");

    banner("MO CUA giua luc dang rua -> tat bom ngay");
    contact(config::kDoor.pin, false);
    tick(60);
    dump("60ms sau khi mo cua");
    tick(200);
    dump("260ms sau khi mo cua");

    banner("mat NTC (dut day) trong luc dang dun");
    contact(config::kDoor.pin, true);
    tick(2000);
    adcRef() = 1023;
    tick(600);
    dump("NTC dut day");

    printf("\n=== ket thuc mo phong t=%lus ===\n", (unsigned long)(msRef() / 1000));
    return 0;
}
