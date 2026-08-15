#include "Ui.h"

#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>

#include "Config.h"

namespace {

// "42.5C", hoac "--.-C" khi khong doc duoc NTC.
void formatTemp(char* dst, size_t size, int16_t deci) {
    if (deci == config::kTempInvalid) {
        snprintf(dst, size, "--.-C");
        return;
    }
    const int16_t whole = static_cast<int16_t>(deci / 10);
    const int16_t frac = static_cast<int16_t>(deci % 10 < 0 ? -(deci % 10) : deci % 10);
    snprintf(dst, size, "%d.%dC", whole, frac);
}

const char* stepName(State st) {
    switch (st) {
        case State::Wash:
            return "WASH";
        case State::Wait:
            return "WAIT";
        case State::Rinse:
            return "RINSE";
        default:
            return "";
    }
}

}  // namespace

bool Ui::begin() {
    ok_ = (lcd_.begin(16, 2) == 0);
    if (ok_) {
        Wire.setClock(400000);
        lcd_.clear();
    }
    return ok_;
}

void Ui::setLine(uint8_t row, const char* text) {
    char padded[17];
    uint8_t i = 0;
    for (; i < 16 && text[i] != '\0'; ++i) {
        padded[i] = text[i];
    }
    for (; i < 16; ++i) {
        padded[i] = ' ';
    }
    padded[16] = '\0';

    if (strcmp(padded, shadow_[row]) == 0) {
        return;  // noi dung khong doi -> khong ton mot lan ghi I2C nao
    }
    strcpy(shadow_[row], padded);
    lcd_.setCursor(0, row);
    lcd_.print(padded);
}

void Ui::show(State st, const Inputs& in, uint16_t remainSec) {
    if (!ok_) {
        return;
    }

    char line1[24] = "";
    char line2[24] = "";
    char temp[10] = "";
    char setpoint[10] = "";
    formatTemp(temp, sizeof(temp), in.tempDeci);

    switch (st) {
        case State::Standby:
            snprintf(line1, sizeof(line1), "POWER OFF");
            snprintf(line2, sizeof(line2), "PRESS POWER");
            break;

        case State::Fill:
            snprintf(line1, sizeof(line1), "FILLING");
            snprintf(line2, sizeof(line2), "TANK%s BOIL%s", in.tankLow ? "--" : "++",
                     in.boilerLow ? "--" : "++");
            break;

        case State::Heat:
            formatTemp(setpoint, sizeof(setpoint), config::kSetpointDeci);
            snprintf(line1, sizeof(line1), "HEATING");
            snprintf(line2, sizeof(line2), "T:%s /%s", temp, setpoint);
            break;

        case State::Ready:
            snprintf(line1, sizeof(line1), "READY");
            snprintf(line2, sizeof(line2), "CLOSE THE DOOR");
            break;

        case State::Wash:
        case State::Wait:
        case State::Rinse:
            snprintf(line1, sizeof(line1), "%-6s%3us", stepName(st), static_cast<unsigned>(remainSec));
            snprintf(line2, sizeof(line2), "T:%s", temp);
            break;

        case State::Finish:
            snprintf(line1, sizeof(line1), "FINISH");
            snprintf(line2, sizeof(line2), "OPEN DOOR");
            break;
    }

    setLine(0, line1);
    setLine(1, line2);
}
