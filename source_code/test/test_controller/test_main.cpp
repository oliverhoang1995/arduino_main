#include <unity.h>

#include "Config.h"
#include "Controller.h"
#include "DebouncedInput.h"

// Unit test chay tren PC (pio test -e native), khong can board.
//
// Chi nham vao cac moc so cu the ma requirement chot cung, va cac cho ma test
// bang tay khong chinh xac hoac khong the lam duoc (tran millis()).

static Controller ctl;
static Inputs in;
static Outputs out;
static uint32_t now;

static int st() { return static_cast<int>(ctl.state()); }
static int stateIs(State s) { return static_cast<int>(s); }

static void reset(uint32_t startMs = 0) {
    now = startMs;
    ctl.begin(now);
    in = Inputs();
    in.tempDeci = 200;  // 20.0 do C
    out = ctl.update(in, now);
}

// Chay may them ms mili giay, dung chu ky dieu khien that (20 ms).
static void step(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i += config::kControlPeriodMs) {
        now += config::kControlPeriodMs;
        out = ctl.update(in, now);
    }
}

static void pressPower() {
    in.powerPressed = true;
    now += config::kControlPeriodMs;
    out = ctl.update(in, now);
    in.powerPressed = false;
}

// Dua may toi READY: 2 bon day, 60.0 do C, cua dang mo.
static void gotoReady(uint32_t startMs = 0) {
    reset(startMs);
    in.tankLow = false;
    in.boilerLow = false;
    in.doorClosed = false;
    in.tempDeci = 600;
    pressPower();
    step(200);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Ready), st());
}

// ---------------------------------------------------------------------------
// 1. Phao thieu nuoc phai giu du 3 giay moi duoc cong nhan (requirement01 muc 8)
// ---------------------------------------------------------------------------
static void test_float_needs_3s_to_confirm() {
    DebouncedInput f(config::kFloatLowConfirmMs, config::kFloatFullConfirmMs);
    f.begin(false, 0);

    for (uint32_t t = 0; t <= 2980; t += 20) {
        f.update(true, t);
    }
    TEST_ASSERT_FALSE(f.isActive());
    f.update(true, 2999);
    TEST_ASSERT_FALSE(f.isActive());
    f.update(true, 3000);
    TEST_ASSERT_TRUE(f.isActive());

    // Chieu nguoc lai chi can 500 ms de dong van kip, khong tran.
    f.update(false, 3000);
    f.update(false, 3499);
    TEST_ASSERT_TRUE(f.isActive());
    f.update(false, 3500);
    TEST_ASSERT_FALSE(f.isActive());
}

// ---------------------------------------------------------------------------
// 2. Phao rung lien tuc thi khong bao gio duoc cong nhan la thieu nuoc
// ---------------------------------------------------------------------------
static void test_float_bouncing_never_confirms() {
    DebouncedInput f(config::kFloatLowConfirmMs, config::kFloatFullConfirmMs);
    f.begin(false, 0);

    for (uint32_t t = 0; t < 10000; t += 20) {
        const bool raw = ((t / 250) % 2) == 0;  // dao trang thai moi 250 ms
        f.update(raw, t);
        TEST_ASSERT_FALSE(f.isActive());
    }
}

// ---------------------------------------------------------------------------
// 3. Vung chet thanh nhiet: ON duoi 82.0, OFF tu 85.0, o giua thi giu nguyen
// ---------------------------------------------------------------------------
static void test_heater_hysteresis() {
    reset();
    in.tempDeci = 500;  // 50.0 do C, chua du dieu kien rua
    pressPower();
    step(200);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Heat), st());
    TEST_ASSERT_TRUE(out.heater);

    in.tempDeci = 830;  // dang ON, chua toi 85.0 -> van ON
    step(6000);
    TEST_ASSERT_TRUE(out.heater);

    in.tempDeci = 850;  // toi nhiet do cai dat -> OFF
    step(6000);
    TEST_ASSERT_FALSE(out.heater);

    in.tempDeci = 830;  // dang OFF, chua tut duoi 82.0 -> van OFF
    step(6000);
    TEST_ASSERT_FALSE(out.heater);

    in.tempDeci = 819;  // tut duoi 82.0 -> ON lai
    step(6000);
    TEST_ASSERT_TRUE(out.heater);
}

// ---------------------------------------------------------------------------
// 4. Boiler thieu nuoc -> cat thanh nhiet NGAY, khong cho min-on 5 giay
// ---------------------------------------------------------------------------
static void test_heater_off_when_boiler_low() {
    reset();
    in.tempDeci = 500;
    pressPower();
    step(200);
    TEST_ASSERT_TRUE(out.heater);

    in.boilerLow = true;
    step(20);  // dung mot chu ky dieu khien
    TEST_ASSERT_FALSE(out.heater);
    TEST_ASSERT_TRUE(out.boilerValve);  // dong thoi mo van cap nuoc Boiler
}

// ---------------------------------------------------------------------------
// 5. Khong doc duoc NTC -> khong dun (chong dun can khi dut day cam bien)
// ---------------------------------------------------------------------------
static void test_heater_off_when_temp_invalid() {
    reset();
    in.tempDeci = 500;
    pressPower();
    step(200);
    TEST_ASSERT_TRUE(out.heater);

    in.tempDeci = config::kTempInvalid;
    step(20);
    TEST_ASSERT_FALSE(out.heater);
}

// ---------------------------------------------------------------------------
// 6. Dang nap nuoc thi chua duoc gia nhiet (requirement01 muc 4)
// ---------------------------------------------------------------------------
static void test_no_heating_while_filling() {
    reset();
    in.tankLow = true;
    in.boilerLow = false;
    in.tempDeci = 200;
    pressPower();
    step(1000);

    TEST_ASSERT_EQUAL_INT(stateIs(State::Fill), st());
    TEST_ASSERT_FALSE(out.heater);
    TEST_ASSERT_TRUE(out.tankValve);
    TEST_ASSERT_FALSE(out.boilerValve);

    in.tankLow = false;  // Tank day -> chuyen sang gia nhiet
    step(40);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Heat), st());
    TEST_ASSERT_TRUE(out.heater);
    TEST_ASSERT_FALSE(out.tankValve);
}

// ---------------------------------------------------------------------------
// 7. Chu trinh dung 65 s / 5 s / 12 s (requirement01 muc 6)
// ---------------------------------------------------------------------------
static void test_cycle_timing() {
    gotoReady();
    in.doorClosed = true;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());
    TEST_ASSERT_TRUE(out.washPump);

    step(64980);  // da chay 64.98 s
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());
    step(20);  // du 65.00 s
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wait), st());
    TEST_ASSERT_FALSE(out.washPump);
    TEST_ASSERT_FALSE(out.rinsePump);

    step(4980);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wait), st());
    step(20);  // du 5.00 s
    TEST_ASSERT_EQUAL_INT(stateIs(State::Rinse), st());
    TEST_ASSERT_TRUE(out.rinsePump);

    step(11980);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Rinse), st());
    step(20);  // du 12.00 s
    TEST_ASSERT_EQUAL_INT(stateIs(State::Finish), st());
    TEST_ASSERT_FALSE(out.rinsePump);

    // Cho nguoi dung mo cua lay do; dong lai thi chay chu trinh moi.
    step(5000);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Finish), st());
    in.doorClosed = false;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Ready), st());
    in.doorClosed = true;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());
}

// ---------------------------------------------------------------------------
// 8. Thieu nuoc giua luc dang rua -> van cap nuoc VA bom van chay
//    (requirement01 muc 8, requirement02 muc 3) - ca quan trong nhat
// ---------------------------------------------------------------------------
static void test_fill_while_washing_does_not_stop_cycle() {
    gotoReady();
    in.doorClosed = true;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());

    step(20000);
    in.tankLow = true;  // phao Tank bao thieu (da xac nhan 3 s o tang Io)
    step(20);
    TEST_ASSERT_TRUE(out.tankValve);
    TEST_ASSERT_TRUE(out.washPump);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());

    in.tankLow = false;
    step(20);
    TEST_ASSERT_FALSE(out.tankValve);
    TEST_ASSERT_TRUE(out.washPump);

    // Nhiet do tut trong luc dang rua: van gia nhiet, chu trinh khong dung.
    in.tempDeci = 700;
    step(20);
    TEST_ASSERT_TRUE(out.heater);
    TEST_ASSERT_TRUE(out.washPump);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());

    // Chay het 65 s -> chu trinh khong he bi dut quang.
    step(45000);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wait), st());

    // Tuong tu voi Boiler khi dang trang.
    step(5000);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Rinse), st());
    in.boilerLow = true;
    step(20);
    TEST_ASSERT_TRUE(out.boilerValve);
    TEST_ASSERT_TRUE(out.rinsePump);
}

// ---------------------------------------------------------------------------
// 9. Chua du 55 do C thi du cua dong cung khong rua (requirement01 muc 6)
// ---------------------------------------------------------------------------
static void test_no_wash_below_ready_temperature() {
    reset();
    in.tankLow = false;
    in.boilerLow = false;
    in.doorClosed = true;
    in.tempDeci = 549;  // 54.9 do C
    pressPower();
    step(5000);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Heat), st());
    TEST_ASSERT_FALSE(out.washPump);

    in.tempDeci = 550;  // dung 55.0 do C
    step(40);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());

    // Nhiet do tut qua tre 1 do C khi may dang cho: quay ve gia nhiet,
    // chu trinh sau phai cho ham lai du 55 do C moi duoc chay.
    in.doorClosed = false;  // huy chu trinh, ve Ready
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Ready), st());
    in.tempDeci = 539;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Heat), st());
    in.doorClosed = true;
    step(200);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Heat), st());
    TEST_ASSERT_FALSE(out.washPump);
}

// ---------------------------------------------------------------------------
// 10. Mo cua giua luc dang trang -> tat bom NGAY, huy chu trinh (Q-03)
// ---------------------------------------------------------------------------
static void test_open_door_aborts_cycle() {
    gotoReady();
    in.doorClosed = true;
    step(20);
    step(65000);  // Wash -> Wait
    step(5000);   // Wait -> Rinse
    TEST_ASSERT_EQUAL_INT(stateIs(State::Rinse), st());
    TEST_ASSERT_TRUE(out.rinsePump);

    in.doorClosed = false;
    step(20);
    TEST_ASSERT_FALSE(out.rinsePump);
    TEST_ASSERT_FALSE(out.washPump);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Ready), st());

    // Dong cua lai: chay chu trinh MOI tu dau (khong chay tiep phan con lai).
    in.doorClosed = true;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());
    TEST_ASSERT_EQUAL_UINT16(65, ctl.stepRemainingSec(now));
}

// ---------------------------------------------------------------------------
// 11. Bam POWER giua chu trinh -> tat het 5 output, ve STANDBY
// ---------------------------------------------------------------------------
static void test_power_button_stops_everything() {
    gotoReady();
    in.doorClosed = true;
    step(20);
    in.tankLow = true;
    step(4000);
    TEST_ASSERT_TRUE(out.washPump);

    pressPower();
    TEST_ASSERT_EQUAL_INT(stateIs(State::Standby), st());
    TEST_ASSERT_FALSE(out.washPump);
    TEST_ASSERT_FALSE(out.rinsePump);
    TEST_ASSERT_FALSE(out.heater);
    TEST_ASSERT_FALSE(out.tankValve);
    TEST_ASSERT_FALSE(out.boilerValve);

    // Bam lan nua thi may chay lai tu dau.
    pressPower();
    TEST_ASSERT_EQUAL_INT(stateIs(State::Fill), st());
}

// ---------------------------------------------------------------------------
// 12. Chay dung khi millis() tran (may thuong mai chay lien tuc 49.7 ngay)
// ---------------------------------------------------------------------------
static void test_millis_overflow() {
    gotoReady(0xFFFFFF00u);  // tran trong luc dang chay
    in.doorClosed = true;
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());

    step(64980);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wash), st());
    step(20);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Wait), st());
    step(5000);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Rinse), st());
    step(12000);
    TEST_ASSERT_EQUAL_INT(stateIs(State::Finish), st());
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_float_needs_3s_to_confirm);
    RUN_TEST(test_float_bouncing_never_confirms);
    RUN_TEST(test_heater_hysteresis);
    RUN_TEST(test_heater_off_when_boiler_low);
    RUN_TEST(test_heater_off_when_temp_invalid);
    RUN_TEST(test_no_heating_while_filling);
    RUN_TEST(test_cycle_timing);
    RUN_TEST(test_fill_while_washing_does_not_stop_cycle);
    RUN_TEST(test_no_wash_below_ready_temperature);
    RUN_TEST(test_open_door_aborts_cycle);
    RUN_TEST(test_power_button_stops_everything);
    RUN_TEST(test_millis_overflow);
    return UNITY_END();
}
