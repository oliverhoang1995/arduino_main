// Chay firmware THAT (file .elf da bien dich cho ATmega2560) tren chip gia lap
// simavr, roi bam nut / dong cua bang phan mem va doc log Serial.
//
// Khac voi tools/sim (bien dich lai code tren PC voi Arduino.h gia), o day chay
// dung ma may AVR that: dung core Arduino that, millis() that, digitalRead()
// that, INPUT_PULLUP that, ADC that.
//
// Build + chay: ./tools/avrsim/run.sh

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avr_adc.h"
#include "avr_ioport.h"
#include "avr_uart.h"
#include "sim_avr.h"
#include "sim_elf.h"

#define F_CPU_HZ 16000000UL

// So do chan Arduino Mega 2560 -> chan cua chip.
//   D9 = PH6   D22 = PA0   D23 = PA1   D24 = PA2   A0 = ADC0
static const char kPortPower = 'H';
static const uint8_t kBitPower = 6;
static const char kPortDoor = 'A';
static const uint8_t kBitDoor = 0;

static avr_t* g_avr = NULL;
static uint32_t g_adcMillivolt = 900;  // ~ADC 184 ~ 63 do C, xem README muc 5.2

// ---------------------------------------------------------------- ghi nhan log
static char g_line[256];
static size_t g_lineLen = 0;
static int g_lineCount = 0;

// Cac moc can kiem tra
static int g_sawStandby = 0;
static int g_sawHeatOrFill = 0;
static int g_sawReady = 0;
static int g_sawWash = 0;

static uint64_t simMs(void) { return g_avr->cycle * 1000ULL / F_CPU_HZ; }

static void onLine(const char* line) {
    ++g_lineCount;
    printf("  [%6llu ms sim] %s\n", (unsigned long long)simMs(), line);

    const char* p = strstr(line, "state=");
    if (p == NULL) {
        return;
    }
    const int st = atoi(p + 6);
    switch (st) {
        case 0:
            g_sawStandby = 1;
            break;
        case 1:
        case 2:
            g_sawHeatOrFill = 1;
            break;
        case 3:
            g_sawReady = 1;
            break;
        case 4:
            g_sawWash = 1;
            break;
        default:
            break;
    }
}

static void onUartByte(struct avr_irq_t* irq, uint32_t value, void* param) {
    (void)irq;
    (void)param;
    const char c = (char)value;
    if (c == '\n' || g_lineLen == sizeof(g_line) - 1) {
        g_line[g_lineLen] = '\0';
        if (g_lineLen > 0) {
            onLine(g_line);
        }
        g_lineLen = 0;
        return;
    }
    if (c != '\r') {
        g_line[g_lineLen++] = c;
    }
}

// simavr hoi "chan ADC nay dang bao nhieu mV?" moi lan firmware doc ADC.
static void onAdcTrigger(struct avr_irq_t* irq, uint32_t value, void* param) {
    (void)irq;
    (void)param;
    union {
        avr_adc_mux_t mux;
        uint32_t raw;
    } u;
    u.raw = value;
    if (u.mux.src == ADC_IRQ_ADC0) {
        avr_raise_irq(avr_io_getirq(g_avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_ADC0), g_adcMillivolt);
    }
}

// ------------------------------------------------------------------ dieu khien
static void setPin(char port, uint8_t bit, int high) {
    avr_raise_irq(avr_io_getirq(g_avr, AVR_IOCTL_IOPORT_GETIRQ(port), bit), high ? 1 : 0);
}

static void runUntilMs(uint64_t targetMs) {
    while (simMs() < targetMs) {
        const int st = avr_run(g_avr);
        if (st == cpu_Done || st == cpu_Crashed) {
            printf("  !! CPU dung o trang thai %d\n", st);
            return;
        }
    }
}

static void step(const char* title) { printf("\n---- t=%llu ms  %s ----\n", (unsigned long long)simMs(), title); }

int main(int argc, char** argv) {
    const char* elfPath = (argc > 1) ? argv[1] : "build/MayRuaChen.ino.elf";
    if (argc > 2) {
        g_adcMillivolt = (uint32_t)atoi(argv[2]);
    }

    elf_firmware_t fw;
    memset(&fw, 0, sizeof(fw));
    if (elf_read_firmware(elfPath, &fw) != 0) {
        fprintf(stderr, "khong doc duoc %s\n", elfPath);
        return 1;
    }
    strcpy(fw.mmcu, "atmega2560");
    fw.frequency = F_CPU_HZ;

    g_avr = avr_make_mcu_by_name(fw.mmcu);
    if (g_avr == NULL) {
        fprintf(stderr, "simavr khong ho tro %s\n", fw.mmcu);
        return 1;
    }
    avr_init(g_avr);
    avr_load_firmware(g_avr, &fw);

    // Tu doc UART0 (= Serial tren Mega), khong de simavr tu in ra.
    uint32_t flags = 0;
    avr_ioctl(g_avr, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
    flags &= ~AVR_UART_FLAG_STDIO;
    avr_ioctl(g_avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
    avr_irq_register_notify(avr_io_getirq(g_avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT), onUartByte,
                            NULL);

    avr_irq_register_notify(avr_io_getirq(g_avr, AVR_IOCTL_ADC_GETIRQ, ADC_IRQ_OUT_TRIGGER), onAdcTrigger,
                            NULL);

    printf("Firmware : %s\n", elfPath);
    printf("Chip     : %s @ %lu Hz\n", fw.mmcu, (unsigned long)fw.frequency);
    printf("A0       : %u mV (gia lap NTC)\n", g_adcMillivolt);
    printf("Chan vao khong noi gi = INPUT_PULLUP = muc CAO = tiep diem HO\n");

    step("khoi dong, chua bam gi");
    runUntilMs(1500);

    step("BAM NUT POWER: keo D9 (PH6) xuong GND 200 ms");
    setPin(kPortPower, kBitPower, 0);
    runUntilMs(1700);
    setPin(kPortPower, kBitPower, 1);
    runUntilMs(3000);

    step("DONG CUA: keo D22 (PA0) xuong GND");
    setPin(kPortDoor, kBitDoor, 0);
    runUntilMs(4000);

    printf("\n==================== KET LUAN ====================\n");
    printf("  So dong log Serial doc duoc : %d\n", g_lineCount);
    printf("  Thay STANDBY (state=0)      : %s\n", g_sawStandby ? "CO" : "KHONG");
    printf("  Bam POWER -> FILL/HEAT      : %s\n", g_sawHeatOrFill ? "CO" : "KHONG");
    printf("  Du nhiet -> READY (state=3) : %s\n", g_sawReady ? "CO" : "KHONG");
    printf("  Dong cua -> WASH  (state=4) : %s\n", g_sawWash ? "CO" : "KHONG");

    const int ok = g_sawStandby && g_sawHeatOrFill;
    printf("\n  KICH NGUON: %s\n", ok ? "DAT" : "KHONG DAT");
    return ok ? 0 : 1;
}
