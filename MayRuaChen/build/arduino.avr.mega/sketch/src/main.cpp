#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/src/main.cpp"
#include <Arduino.h>

#include "Config.h"
#include "Controller.h"
#include "Io.h"
#include "Ntc.h"
#include "Ui.h"

// May rua chen thuong mai - Arduino Mega 2560.
// Tham chieu: plans_02/02-design.md
//
// File nay chi lam viec noi day. Toan bo logic nam trong Controller.

static Io io;
static Ntc ntc;
static Controller controller;
static Ui ui;

static Inputs lastInputs;
static State lastState = State::Standby;

static uint32_t tInput = 0;
static uint32_t tNtc = 0;
static uint32_t tControl = 0;
static uint32_t tUi = 0;

// Phep tru unsigned: dung ca khi millis() tran sau 49.7 ngay.
static bool due(uint32_t& last, uint16_t periodMs, uint32_t nowMs) {
    if (static_cast<uint32_t>(nowMs - last) < periodMs) {
        return false;
    }
    last = nowMs;
    return true;
}

// In mot dong CSV moi khi co gi do thay doi. Khong in dinh ky de khong lam
// cham vong lap; du de doi chieu khi test tren ban va khi lap dat may.
static void logOnChange(uint32_t nowMs, const Inputs& in, const Outputs& out, State st) {
    static bool first = true;
    static State prevState = State::Standby;
    static Inputs prevIn;
    static Outputs prevOut;

    const bool inChanged = in.doorClosed != prevIn.doorClosed || in.tankFull != prevIn.tankFull ||
                           in.boilerFull != prevIn.boilerFull;
    const bool outChanged = out.washPump != prevOut.washPump || out.rinsePump != prevOut.rinsePump ||
                            out.heater != prevOut.heater || out.tankValve != prevOut.tankValve ||
                            out.boilerValve != prevOut.boilerValve;

    if (!first && st == prevState && !inChanged && !outChanged) {
        return;
    }
    first = false;
    prevState = st;
    prevIn = in;
    prevOut = out;

    Serial.print(nowMs);
    Serial.print(F(",state="));
    Serial.print(static_cast<uint8_t>(st));
    Serial.print(F(",temp="));
    Serial.print(in.tempDeci);
    Serial.print(F(",door="));
    Serial.print(in.doorClosed ? 'C' : 'O');
    Serial.print(F(",tank="));
    Serial.print(in.tankFull ? 'F' : 'L');
    Serial.print(F(",boiler="));
    Serial.print(in.boilerFull ? 'F' : 'L');
    Serial.print(F(",out="));
    Serial.print(out.washPump ? 'W' : '-');
    Serial.print(out.rinsePump ? 'R' : '-');
    Serial.print(out.heater ? 'H' : '-');
    Serial.print(out.tankValve ? 'T' : '-');
    Serial.println(out.boilerValve ? 'B' : '-');
}

void setup() {
    Serial.begin(115200);

    const uint32_t now = millis();
    io.begin(now);  // ghi muc OFF cho 5 output truoc khi lam bat cu viec gi khac
    ntc.begin();
    controller.begin(now);

    if (!ui.begin()) {
        Serial.println(F("LCD not found - may van chay, chi khong hien thi"));
    }

    lastInputs = io.read(ntc.deci());
    lastState = controller.state();
    tInput = tNtc = tControl = tUi = now;
}

void loop() {
    const uint32_t now = millis();

    if (due(tInput, config::kInputPeriodMs, now)) {
        io.poll(now);
    }
    if (due(tNtc, config::kNtcPeriodMs, now)) {
        ntc.sample();
    }
    if (due(tControl, config::kControlPeriodMs, now)) {
        lastInputs = io.read(ntc.deci());
        const Outputs out = controller.update(lastInputs, now);
        io.write(out);
        logOnChange(now, lastInputs, out, controller.state());

        // Doi state (dong cua -> WASH, het gio -> RINSE...) phai len LCD NGAY,
        // khong doi het chu ky 250 ms cua task UI.
        if (controller.state() != lastState) {
            lastState = controller.state();
            ui.show(lastState, lastInputs, controller.stepRemainingSec(now));
            tUi = now;
        }
    }
    if (due(tUi, config::kUiPeriodMs, now)) {
        ui.show(controller.state(), lastInputs, controller.stepRemainingSec(now));
    }
}
