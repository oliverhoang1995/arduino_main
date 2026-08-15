// Do chinh xac: (1) tre tat bom khi mo cua, (2) sai so quy doi NTC,
// (3) thoi gian xac nhan phao o tang Io, (4) loc nhieu ADC.
#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "Config.h"
#include "Ntc.h"

void setup();
void loop();

static void contact(uint8_t pin, bool closed) { pinRef()[pin] = closed ? LOW : HIGH; }
static bool out(const config::OutputPin& c) {
    return c.activeLow ? (outRef()[c.pin] == LOW) : (outRef()[c.pin] == HIGH);
}
static void tick(uint32_t ms) {
    for (uint32_t i = 0; i < ms; ++i) {
        msRef() += 1;
        loop();
    }
}
static int adcForTemp(double tC) {
    const double t = tC + 273.15;
    const double r = 10000.0 * exp(3950.0 * (1.0 / t - 1.0 / 298.15));
    return (int)(1023.0 * r / (10000.0 + r) + 0.5);
}
static uint32_t waitUntilPumpOff(uint32_t limitMs) {
    for (uint32_t i = 0; i < limitMs; ++i) {
        msRef() += 1;
        loop();
        if (!out(config::kWashPump) && !out(config::kRinsePump)) {
            return i + 1;
        }
    }
    return limitMs;
}
static uint32_t waitUntilValve(bool wanted, uint32_t limitMs) {
    for (uint32_t i = 0; i < limitMs; ++i) {
        msRef() += 1;
        loop();
        if (out(config::kTankValve) == wanted) {
            return i + 1;
        }
    }
    return limitMs;
}

int main() {
    printf("=== 1. Quy doi NTC (so voi bang trong plans_02/02-design.md) ===\n");
    {
        struct Case { int adc; double expectC; };
        const Case cases[] = {{773, 0.0}, {569, 20.0}, {512, 25.0}, {356, 40.0},
                              {237, 55.0}, {155, 70.0}, {101, 85.0}, {78, 95.0}};
        Ntc ntc;
        for (const Case& c : cases) {
            adcRef() = c.adc;
            ntc.begin();
            for (int i = 0; i < 60; ++i) {
                ntc.sample();
            }
            const double got = ntc.deci() / 10.0;
            printf("  ADC %4d -> %5.1f C  (bang: %5.1f C, lech %+.1f)\n", c.adc, got, c.expectC,
                   got - c.expectC);
        }
        adcRef() = 1023;
        ntc.begin();
        ntc.sample();
        printf("  ADC 1023 (dut day)  -> %s\n", ntc.deci() == config::kTempInvalid ? "INVALID (dung)" : "SAI");
        adcRef() = 0;
        ntc.begin();
        ntc.sample();
        printf("  ADC    0 (ngan mach)-> %s\n", ntc.deci() == config::kTempInvalid ? "INVALID (dung)" : "SAI");
    }

    printf("\n=== 2. Loc nhieu: 1 xung nhieu don le giua chuoi mau on dinh ===\n");
    {
        Ntc ntc;
        adcRef() = adcForTemp(80.0);
        ntc.begin();
        for (int i = 0; i < 60; ++i) {
            ntc.sample();
        }
        const double before = ntc.deci() / 10.0;
        adcRef() = 900;  // xung nhieu 1 mau (tuong duong ~5 do C)
        ntc.sample();
        adcRef() = adcForTemp(80.0);
        const double during = ntc.deci() / 10.0;
        ntc.sample();
        ntc.sample();
        printf("  truoc %.1f C -> sau 1 mau nhieu %.1f C (lech %+.1f)\n", before, during, during - before);
    }

    printf("\n=== 3. Tre tat bom khi mo cua (yeu cau <= 150 ms) ===\n");
    {
        for (int i = 0; i < 80; ++i) {
            pinRef()[i] = HIGH;
        }
        msRef() = 0;
        contact(config::kDoor.pin, false);
        contact(config::kTankFloat.pin, false);    // 2 bon day
        contact(config::kBoilerFloat.pin, false);
        adcRef() = adcForTemp(80.0);
        setup();

        contact(config::kBtnPower.pin, true);
        tick(120);
        contact(config::kBtnPower.pin, false);
        tick(500);  // -> READY

        contact(config::kDoor.pin, true);
        tick(500);
        printf("  dang WASH: wash=%d\n", out(config::kWashPump));

        contact(config::kDoor.pin, false);
        const uint32_t latency = waitUntilPumpOff(2000);
        printf("  mo cua -> bom tat sau %lu ms  %s\n", (unsigned long)latency,
               latency <= 150 ? "OK" : "QUA CHAM");
    }

    printf("\n=== 4. Xac nhan phao o tang Io (yeu cau: 3000 ms / 500 ms) ===\n");
    {
        for (int i = 0; i < 80; ++i) {
            pinRef()[i] = HIGH;
        }
        msRef() = 0;
        contact(config::kDoor.pin, false);
        contact(config::kTankFloat.pin, false);
        contact(config::kBoilerFloat.pin, false);
        adcRef() = adcForTemp(80.0);
        setup();
        contact(config::kBtnPower.pin, true);
        tick(120);
        contact(config::kBtnPower.pin, false);
        tick(500);

        contact(config::kTankFloat.pin, true);
        const uint32_t tOpen = waitUntilValve(true, 6000);
        printf("  phao bao thieu -> van mo sau %lu ms (3000 + <= 40 ms quet)\n", (unsigned long)tOpen);
        contact(config::kTankFloat.pin, false);
        const uint32_t tClose = waitUntilValve(false, 3000);
        printf("  phao bao du    -> van dong sau %lu ms (500 + <= 40 ms quet)\n", (unsigned long)tClose);

        // rung phao: bat/tat moi 250 ms trong 30 giay
        bool opened = false;
        for (uint32_t t = 0; t < 30000; ++t) {
            contact(config::kTankFloat.pin, ((t / 250) % 2) == 0);
            msRef() += 1;
            loop();
            if (out(config::kTankValve)) {
                opened = true;
            }
        }
        printf("  rung phao 2 Hz trong 30 s -> van %s\n", opened ? "CO MO (SAI)" : "khong mo (dung)");
    }

    printf("\n=== 5. Tai CPU: so lan loop() moi giay o AVR 16 MHz (uoc luong) ===\n");
    printf("  (chi kiem tra logic khong chan; do that phai do tren board)\n");
    return 0;
}
