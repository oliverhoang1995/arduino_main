#include "Controller.h"

#include "Config.h"

namespace {

// Cap nuoc va gia nhiet chay o moi state tru Standby.
bool machineRunning(State s) { return s != State::Standby; }

// Thanh nhiet chi duoc phep chay sau khi CA 2 bon da day (requirement01 muc 4),
// tuc la tu state Heat tro di.
bool heatingAllowed(State s) { return s != State::Standby && s != State::Fill; }

}  // namespace

void Controller::begin(uint32_t nowMs) {
    state_ = State::Standby;
    stepStartMs_ = nowMs;
    ready_ = false;
    heaterOn_ = false;
    // Lui moc thoi gian de lan bat thanh nhiet dau tien khong phai cho min-off.
    heaterChangedMs_ = nowMs - config::kHeaterMinOffMs;
}

void Controller::enter(State next, uint32_t nowMs) {
    state_ = next;
    stepStartMs_ = nowMs;
}

// Tre 1 do C quanh nguong 55 do C de may khong nhay qua lai giua Heat va Ready.
void Controller::updateReady(int16_t tempDeci) {
    if (tempDeci == config::kTempInvalid) {
        ready_ = false;
        return;
    }
    if (ready_) {
        if (tempDeci < config::kReadyDeci - config::kReadyHystDeci) {
            ready_ = false;
        }
    } else if (tempDeci >= config::kReadyDeci) {
        ready_ = true;
    }
}

bool Controller::updateHeater(const Inputs& in, uint32_t nowMs, bool enabled) {
    // Dieu kien cung: khong co nuoc trong Boiler hoac khong doc duoc nhiet do
    // thi tuyet doi khong dun (thanh nhiet ho nuoc se chay).
    const bool safeToHeat = enabled && in.boilerFull && in.tempDeci != config::kTempInvalid;

    bool want;
    if (!safeToHeat) {
        want = false;
    } else if (heaterOn_) {
        want = in.tempDeci < config::kSetpointDeci;
    } else {
        want = in.tempDeci < config::kSetpointDeci - config::kHeaterHystDeci;
    }

    if (want != heaterOn_) {
        // Gioi han tan so dong cat contactor. Rieng truong hop tat vi ly do an
        // toan thi cat ngay, khong cho min-on.
        const bool safetyOff = !want && !safeToHeat;
        const uint32_t minMs = heaterOn_ ? config::kHeaterMinOnMs : config::kHeaterMinOffMs;
        if (safetyOff || static_cast<uint32_t>(nowMs - heaterChangedMs_) >= minMs) {
            heaterOn_ = want;
            heaterChangedMs_ = nowMs;
        }
    }
    return heaterOn_;
}

uint32_t Controller::stepDurationMs() const {
    switch (state_) {
        case State::Wash:
            return config::kWashMs;
        case State::Wait:
            return config::kWaitMs;
        case State::Rinse:
            return config::kRinseMs;
        default:
            return 0;
    }
}

uint16_t Controller::stepRemainingSec(uint32_t nowMs) const {
    const uint32_t total = stepDurationMs();
    if (total == 0) {
        return 0;
    }
    const uint32_t elapsed = static_cast<uint32_t>(nowMs - stepStartMs_);
    if (elapsed >= total) {
        return 0;
    }
    return static_cast<uint16_t>((total - elapsed + 999) / 1000);
}

Outputs Controller::update(const Inputs& in, uint32_t nowMs) {
    // 1. Nut POWER: bat / tat may, uu tien tren moi thu khac.
    if (in.powerPressed) {
        enter(state_ == State::Standby ? State::Fill : State::Standby, nowMs);
    }

    updateReady(in.tempDeci);

    // 2. State machine tuan tu (plans_02/02-design.md muc 2).
    const uint32_t elapsed = static_cast<uint32_t>(nowMs - stepStartMs_);
    const bool tanksFull = in.tankFull && in.boilerFull;

    switch (state_) {
        case State::Standby:
            break;

        case State::Fill:
            if (tanksFull) {
                enter(State::Heat, nowMs);
            }
            break;

        case State::Heat:
            if (ready_) {
                enter(State::Ready, nowMs);
            }
            break;

        case State::Ready:
            if (!ready_) {
                enter(State::Heat, nowMs);
            } else if (in.doorClosed && tanksFull) {
                enter(State::Wash, nowMs);
            }
            break;

        case State::Wash:
            // Cua mo: tat bom ngay, huy chu trinh (Q-03).
            if (!in.doorClosed) {
                enter(State::Ready, nowMs);
            } else if (elapsed >= config::kWashMs) {
                enter(State::Wait, nowMs);
            }
            break;

        case State::Wait:
            if (!in.doorClosed) {
                enter(State::Ready, nowMs);
            } else if (elapsed >= config::kWaitMs) {
                enter(State::Rinse, nowMs);
            }
            break;

        case State::Rinse:
            if (!in.doorClosed) {
                enter(State::Ready, nowMs);
            } else if (elapsed >= config::kRinseMs) {
                enter(State::Finish, nowMs);
            }
            break;

        case State::Finish:
            // Cho nguoi dung mo cua lay do; dong cua lai se chay chu trinh moi.
            if (!in.doorClosed) {
                enter(State::Ready, nowMs);
            }
            break;
    }

    // 3. Cap nuoc + gia nhiet: chay song song, KHONG phu thuoc state dang o dau
    //    (requirement01 muc 8, requirement02 muc 3).
    Outputs out;
    const bool running = machineRunning(state_);
    out.tankValve = running && !in.tankFull;
    out.boilerValve = running && !in.boilerFull;
    out.heater = updateHeater(in, nowMs, heatingAllowed(state_));

    // 4. Bom cua chu trinh rua.
    out.washPump = (state_ == State::Wash);
    out.rinsePump = (state_ == State::Rinse);

    return out;
}
