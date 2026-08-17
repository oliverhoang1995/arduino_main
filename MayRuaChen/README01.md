# Máy rửa chén thương mại — Arduino Mega 2560

Tài liệu kỹ thuật đầy đủ. Chỉ cần nạp máy bằng Arduino IDE thì đọc `README.md`.

Firmware theo `../plans_02/`. Requirement gốc: `../documents/requirement01.md`,
`../documents/requirement02.md`.

---

## 1. Build / nạp / test

Có 3 cách, dùng chung một bản mã nguồn `src/`. **Không dùng thư viện bên thứ ba nào** —
LCD I2C là driver tự viết (`src/LcdI2c.cpp`) trên `Wire.h` có sẵn.

**a) Arduino IDE** — cách mặc định, xem `README.md`. Mở `MayRuaChen.ino`, chọn board
Mega 2560, Upload. IDE chỉ biên dịch `MayRuaChen.ino` + `src/` (đệ quy) và **bỏ qua**
`test/`, `tools/`.

**b) arduino-cli** — build không cần mở IDE:

```bash
arduino-cli compile -b arduino:avr:mega .
arduino-cli upload  -b arduino:avr:mega -p /dev/tty.usbmodemXXXX .
```

**c) Kiểm chứng trên PC** — 13 unit test + đo số liệu + mô phỏng end-to-end, không cần
board, chỉ cần một trình biên dịch C++17:

```bash
./tools/sim/run.sh
```

PlatformIO là tuỳ chọn (`pip install platformio`), `platformio.ini` đã cấu hình sẵn:

```bash
pio run -e mega2560 -t upload    # nạp board
pio test -e native               # unit test trên PC
```

---

## 2. File

| File                      | Trách nhiệm                                                                                                       |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `src/Config.h`          | **Toàn bộ** pin mapping + tham số thời gian/nhiệt độ. Đổi phần cứng chỉ sửa file này            |
| `src/DebouncedInput.h`  | Debounce có thời gian xác nhận riêng cho từng chiều (phao: 3 s / 0.5 s)                                      |
| `src/Io.h/.cpp`         | Ranh giới duy nhất với phần cứng: đọc 4 input số, ghi 5 output                                              |
| `src/Ntc.h/.cpp`        | NTC 10K → 0.1 °C (median 5 mẫu → EMA → công thức β)                                                         |
| `src/Controller.h/.cpp` | **Toàn bộ logic**: state machine + auto-fill + điều khiển thanh nhiệt. Không phụ thuộc `Arduino.h` |
| `src/Ui.h/.cpp`         | LCD 16×2 I2C, chỉ ghi dòng có nội dung thay đổi                                                              |
| `src/LcdI2c.h/.cpp`     | Driver HD44780 qua PCF8574, chỉ dùng`Wire.h` — thay cho thư viện `hd44780`                                 |
| `src/main.cpp`          | Nối dây + vòng lặp 4 task. Không chứa logic                                                                   |
| `test/test_controller/` | 12 unit test                                                                                                        |

---

## 3. Nối dây

### Input — `INPUT_PULLUP`, tiếp điểm về GND

| Tín hiệu      | Pin | ACTIVE (tiếp điểm đóng) nghĩa là                    |
| --------------- | --- | ---------------------------------------------------------- |
| Cảm biến cửa | D22 | cửa**đóng**                                       |
| Phao Tank       | D23 | Tank**đủ** nước                                  |
| Phao Boiler     | D24 | Boiler**đủ** nước                                |
| Nút POWER      | D9  | đang nhấn                                                |
| NTC 10K         | A0  | 5V — Rs 10 kΩ 1% — A0 — NTC — GND, tụ 100 nF tại A0 |

Nếu tiếp điểm là **NC** thay vì NO: đổi `invert` thành `true` trong `Config.h`, không sửa code.

> **Quy ước tín hiệu:** mọi input mặc định là `false` (chưa có tín hiệu) — cửa MỞ, 2 bồn THIẾU nước.
> Chỉ khi tiếp điểm thực sự đóng và giữ đủ thời gian xác nhận thì mới thành `true`.
> Vì vậy **phao phải đấu sao cho tiếp điểm ĐÓNG khi nước ĐẦY** (phao NO, nhấc lên thì đóng).
> Đứt dây / tuột giắc = thiếu nước → máy mở van cấp và **cấm gia nhiệt**; đây là chiều an toàn,
> không bao giờ đun cạn.

D25 (MODE), D27 (UP), D28 (DOWN): chưa dùng (Q-02 — sẽ làm sau).

### Output — qua relay module (mặc định active-LOW)

| Thiết bị             | Pin                   |
| ---------------------- | --------------------- |
| Bơm rửa              | D2                    |
| Bơm tráng            | D3                    |
| Thanh nhiệt Boiler    | D4                    |
| Van cấp nước Tank   | D5                    |
| Van cấp nước Boiler | D6                    |
| LCD I2C                | D20 (SDA) / D21 (SCL) |

D7, D8 (bơm hóa chất): điều khiển ngoài Arduino (Q-01).

Nếu relay là active-HIGH: đổi `activeLow` thành `false` trong `Config.h`.

### 3 lưu ý điện (bắt buộc)

1. **Thanh nhiệt phải qua SSR hoặc contactor.** Relay của module 8 kênh chỉ dùng để kích cuộn contactor.
2. **Mỗi van solenoid AC cần snubber RC (100 Ω + 100 nF)** song song tiếp điểm — thiếu snubber là nguyên
   nhân phổ biến nhất gây treo LCD / reset MCU.
3. **Nguồn 5 V riêng ≥ 2 A cho Arduino + LCD**, tách khỏi nguồn cấp cuộn relay. Dây NTC dùng cáp có vỏ
   chống nhiễu, đi tách dây động lực ≥ 10 cm.

---

## 4. Hành vi

```
STANDBY ──POWER──► FILL ──2 bồn đầy──► HEAT ──≥55°C──► READY ──cửa đóng──► WASH 65s
                                                          ▲                    │
                                            cửa mở ───────┤              WAIT 5s
                                                          │                    │
                                              FINISH ◄────┴──── RINSE 12s ◄────┘
```

Chạy **song song ở mọi trạng thái** (trừ STANDBY):

- Phao báo thiếu giữ đủ 3 s → mở van bồn đó; báo đủ 0.5 s → đóng van.
- Thanh nhiệt: ON dưới 82.0 °C, OFF từ 85.0 °C, min ON/OFF 5 s.
  Chỉ chạy khi **phao Boiler báo đủ nước** và **đọc được NTC** — nếu không thì tắt ngay.
- Đang rửa mà thiếu nước hoặc nhiệt độ tụt: vẫn cấp nước / vẫn gia nhiệt, **không dừng chu trình**.
- Mở cửa giữa chu trình: tắt bơm ngay, huỷ chu trình, về READY. Đóng cửa lại → chu trình mới từ đầu.
- Bấm POWER bất kỳ lúc nào → tắt cả 5 output, về STANDBY.

Đổi tham số (65 s / 5 s / 12 s / 55 °C / 85 °C / 3 s...) trong `src/Config.h`.

---

## 5. Log Serial (115200)

In một dòng CSV **mỗi khi có thay đổi** (không in định kỳ):

```
207500,state=4,temp=858,door=C,tank=F,boiler=F,out=W----
```

- `state`: 0=STANDBY 1=FILL 2=HEAT 3=READY 4=WASH 5=WAIT 6=RINSE 7=FINISH
- `temp`: 0.1 °C (`-32768` = không đọc được NTC)
- `door`: C=đóng O=mở · `tank`/`boiler`: F=đủ L=thiếu
- `out`: `W`=bơm rửa `R`=bơm tráng `H`=thanh nhiệt `T`=van Tank `B`=van Boiler (`-` = tắt)

Paste vào Excel để dựng lại toàn bộ diễn biến khi cần điều tra sự cố hiện trường.

---

## 6. Đã verify những gì

Chi tiết ở `VERIFICATION.md`. Tóm tắt:

| Hạng mục                                         | Kết quả                   |
| -------------------------------------------------- | --------------------------- |
| 12 unit test (logic, chạy trên PC)               | 12/12 pass                  |
| Biên dịch`-Wall -Wextra -Wshadow`              | 0 warning                   |
| Mô phỏng end-to-end toàn bộ luồng requirement | đúng                      |
| Chu trình 65 s / 5 s / 12 s                       | chính xác tuyệt đối    |
| Mở cửa → tắt bơm                              | 80 ms (yêu cầu ≤ 150 ms) |
| Xác nhận phao thiếu / đủ nước               | 3020 ms / 520 ms            |
| Rung phao 2 Hz trong 30 s                          | van không mở lần nào    |
| Sai số quy đổi NTC (20–85 °C)                 | ≤ 0.5 °C                  |
| Chạy đúng khi`millis()` tràn (49.7 ngày)    | đúng                      |

**Chưa verify được trên máy này** (không có toolchain AVR / không có board): dung lượng flash-SRAM thật,
thời gian vòng lặp thật, LCD thật, chống nhiễu thật. Xem `VERIFICATION.md` §3 để biết cần làm gì tiếp.
