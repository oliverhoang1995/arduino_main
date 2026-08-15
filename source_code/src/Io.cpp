#include "Io.h"

#include <Arduino.h>

static_assert(config::kNtcPin == A0, "config::kNtcPin phai la chan A0 cua Mega 2560");

namespace {

// Tiep diem noi ve GND + INPUT_PULLUP => LOW nghia la tiep diem DANG DONG.
bool readRaw(const config::InputPin& cfg) {
    const bool closed = (digitalRead(cfg.pin) == LOW);
    return closed != cfg.invert;
}

void setupInput(const config::InputPin& cfg) { pinMode(cfg.pin, INPUT_PULLUP); }

void setOutput(const config::OutputPin& cfg, bool on) {
    digitalWrite(cfg.pin, (on != cfg.activeLow) ? HIGH : LOW);
}

// Ghi muc OFF TRUOC pinMode(OUTPUT) de relay active-LOW khong nhap mot nhip
// luc cap nguon.
void setupOutput(const config::OutputPin& cfg) {
    setOutput(cfg, false);
    pinMode(cfg.pin, OUTPUT);
    setOutput(cfg, false);
}

}  // namespace

void Io::begin(uint32_t nowMs) {
    setupOutput(config::kWashPump);
    setupOutput(config::kRinsePump);
    setupOutput(config::kHeater);
    setupOutput(config::kTankValve);
    setupOutput(config::kBoilerValve);

    setupInput(config::kDoor);
    setupInput(config::kTankFloat);
    setupInput(config::kBoilerFloat);
    setupInput(config::kBtnPower);

    // Nhan trang thai thuc te luc khoi dong: bon dang can thi biet ngay,
    // khong phai cho 3 giay xac nhan.
    door_.begin(readRaw(config::kDoor), nowMs);
    tank_.begin(readRaw(config::kTankFloat), nowMs);
    boiler_.begin(readRaw(config::kBoilerFloat), nowMs);
    power_.begin(readRaw(config::kBtnPower), nowMs);
    powerEvent_ = false;
}

void Io::poll(uint32_t nowMs) {
    door_.update(readRaw(config::kDoor), nowMs);
    tank_.update(readRaw(config::kTankFloat), nowMs);
    boiler_.update(readRaw(config::kBoilerFloat), nowMs);
    power_.update(readRaw(config::kBtnPower), nowMs);

    if (power_.rose()) {
        powerEvent_ = true;
    }
}

Inputs Io::read(int16_t tempDeci) {
    Inputs in;
    in.doorClosed = door_.isActive();
    in.tankLow = tank_.isActive();
    in.boilerLow = boiler_.isActive();
    in.powerPressed = powerEvent_;
    in.tempDeci = tempDeci;
    powerEvent_ = false;
    return in;
}

void Io::write(const Outputs& out) const {
    setOutput(config::kWashPump, out.washPump);
    setOutput(config::kRinsePump, out.rinsePump);
    setOutput(config::kHeater, out.heater);
    setOutput(config::kTankValve, out.tankValve);
    setOutput(config::kBoilerValve, out.boilerValve);
}
