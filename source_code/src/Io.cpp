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

    // MAC DINH TAT CA LA false: luc khoi dong may coi nhu chua co tin hieu nao
    // (cua MO, ca 2 bon THIEU nuoc, khong ai bam nut). Chi khi tiep diem thuc su
    // dong va giu du thoi gian xac nhan thi poll() moi doi sang true.
    //
    // Co y KHONG doc chan tai day: pull-up noi 20-50k nap dien dung day rat cham,
    // lan doc dau tien ngay sau pinMode() de ra LOW gia neu day di dai trong tu.
    // Xuat phat tu false thi khong the sai theo chieu nguy hiem: bon bi coi la
    // thieu nuoc => cam gia nhiet, khong bao gio dun can.
    door_.begin(false, nowMs);
    tank_.begin(false, nowMs);
    boiler_.begin(false, nowMs);
    power_.begin(false, nowMs);
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
    in.tankFull = tank_.isActive();
    in.boilerFull = boiler_.isActive();
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
