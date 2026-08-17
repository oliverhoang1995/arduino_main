#line 1 "/Users/thaoluong/Documents/01.work/projects/outsourcing/arduino_main/MayRuaChen/README.md"
# MayRuaChen — bản dùng cho Arduino IDE

Đây là **bản sketch của Arduino IDE**, nội dung code giống hệt `source_code/src/`
(bản gốc dùng PlatformIO). Mở bằng Arduino IDE là build + nạp được ngay,
**không cần cài thư viện nào**.

---

## 1. Dùng thế nào

1. Copy nguyên thư mục `MayRuaChen/` này vào thư mục sketchbook:
   - macOS / Linux: `~/Documents/Arduino/`
   - Windows: `Documents\Arduino\`
2. Mở `MayRuaChen.ino` bằng Arduino IDE.
3. **Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560**
   **Tools → Processor → ATmega2560 (Mega 2560)**
4. Chọn Port → bấm **Upload**.
5. **Tools → Serial Monitor**, đặt **115200 baud** để xem log.

Tên thư mục và tên file `.ino` phải luôn trùng nhau. Đổi tên thì đổi cả hai.

---

## 2. Vì sao file `.ino` để trống

Arduino IDE bắt buộc phải có file `.ino`, nhưng nó cũng **biên dịch đệ quy thư
mục con tên `src/`**. Nên toàn bộ code nằm trong `src/`, còn `.ino` chỉ là chỗ
ghi chú. Cách này giữ file y nguyên bản gốc PlatformIO — không phải sửa một
dòng `#include` nào, và tránh được việc IDE tự chèn function prototype vào file
`.ino` (hay gây lỗi khó hiểu với các hàm nhận `Inputs` / `Outputs` / `State`).

`setup()` và `loop()` nằm ở `src/main.cpp`.

---

## 3. Thư viện: không cần cài gì

| Thư viện | Nguồn                                        |
| ---------- | --------------------------------------------- |
| `Wire.h` | có sẵn trong Arduino IDE                    |
| LCD I2C    | `src/LcdI2c.cpp` — tự viết trong dự án |

LCD 16×2 chạy qua module I2C PCF8574. Driver tự dò địa chỉ **0x27** rồi **0x3F**
(hai địa chỉ phổ biến nhất). Không thấy LCD thì máy **vẫn chạy bình thường**,
chỉ không hiển thị, và Serial in ra `LCD not found`.

Nếu LCD sáng đèn nhưng không ra chữ / ra ký tự rác:

| Hiện tượng                       | Sửa ở đâu                                                       |
| ----------------------------------- | ------------------------------------------------------------------- |
| Không thấy LCD (địa chỉ khác) | `src/Config.h` → `kLcdAddresses`                               |
| Ký tự rác, cáp I2C đi dài     | `src/Config.h` → `kI2cClockHz` hạ xuống `100000`           |
| Sáng đèn nhưng trắng trơn     | vặn biến trở tương phản trên lưng module                    |
| Module nối chân khác chuẩn      | 4 hằng số`kBitRs/kBitEn/kBitBacklight` đầu `src/LcdI2c.cpp` |

---

## 4. Nối dây (tóm tắt)

Đầy đủ + 3 lưu ý điện bắt buộc: xem `../source_code/README.md` mục 3.

| Vào (INPUT_PULLUP, tiếp điểm về GND) | Pin |  | Ra (relay active-LOW)  | Pin       |
| ----------------------------------------- | --- | - | ---------------------- | --------- |
| Cảm biến cửa (ACTIVE = đóng)         | D22 |  | Bơm rửa              | D2        |
| Phao Tank (ACTIVE = đủ nước)          | D23 |  | Bơm tráng            | D3        |
| Phao Boiler (ACTIVE = đủ nước)        | D24 |  | Thanh nhiệt Boiler    | D4        |
| Nút POWER                                | D9  |  | Van cấp nước Tank   | D5        |
| NTC 10K (sơ đồ ở mục 5)              | A0  |  | Van cấp nước Boiler | D6        |
|                                           |     |  | LCD I2C SDA / SCL      | D20 / D21 |

Đổi pin, đổi tiếp điểm NO↔NC, đổi relay active-HIGH: **chỉ sửa `src/Config.h`**.

> Phao đấu sao cho **tiếp điểm ĐÓNG khi nước ĐẦY**. Chưa có tín hiệu (đứt dây, tuột giắc,
> vừa cấp nguồn) = THIẾU nước → mở van, cấm gia nhiệt. Đây là chiều an toàn.

Đấu dây xong mà một tín hiệu không ăn: nạp sketch chẩn đoán `../KiemTraChan/KiemTraChan.ino`,
nó in ra đúng số hiệu chân đang bị nối GND — biết ngay là cắm nhầm lỗ hay đứt dây.
Thử xong nạp lại `MayRuaChen.ino`.

---

## 5. Mạch phân áp NTC ở chân A0

### 5.1. Mạch thật

```
   +5V ───────┬─────────────────────────
              │
             ┌┴┐
             │ │   Rs = 10 kΩ  1%   (điện trở cố định)
             │ │
             └┬┘
              │
    A0 ───────┼──────────────┬──────────   ← Arduino đo điện áp ở đây
              │              │
             ┌┴┐            ═╪═  C = 100 nF
             │ │   NTC 10K   │   (tụ lọc nhiễu, đặt sát chân A0)
             │ │   β = 3950  │
             └┬┘             │
              │              │
   GND ───────┴──────────────┴──────────
```

**NTC nằm ở nhánh dưới.** Hệ quả cần nhớ khi soi log: nước càng nóng → điện trở NTC càng
giảm → điện áp A0 càng thấp → **số ADC càng nhỏ**. Ngược chiều trực giác.

Công thức chương trình đang dùng (`src/Ntc.cpp`):

```
Rntc = 10000 × ADC / (1023 − ADC)
1/T  = 1/298.15 + ln(Rntc/10000) / 3950
```

Dây NTC dùng cáp có vỏ chống nhiễu, đi tách dây động lực ≥ 10 cm.

### 5.2. Test bàn — điện trở cố định

Thay đúng con NTC bằng một điện trở thường, **giữ nguyên Rs 10 kΩ ở nhánh trên**:

```
   +5V ───────┬──────────
              │
             ┌┴┐
             │ │   Rs = 10 kΩ      ← giữ nguyên, luôn luôn 10k
             └┬┘
              │
    A0 ───────┼──────────
              │
             ┌┴┐
             │ │   Rtest           ← đổi con này để giả lập nhiệt độ
             └┬┘
              │
   GND ───────┴──────────
```

| Rtest             | Điện áp tại A0 | ADC           | Nhiệt độ máy đọc | Máy sẽ làm gì                                              |
| ----------------- | ------------------ | ------------- | ---------------------- | -------------------------------------------------------------- |
| 10 kΩ            | 2.50 V             | 512           | 25 °C                 | HEAT, thanh nhiệt bật                                        |
| 4.7 kΩ           | 1.60 V             | 327           | 43 °C                 | HEAT, thanh nhiệt bật                                        |
| **3.0 kΩ** | **1.15 V**   | **235** | **55 °C**       | vừa chạm ngưỡng →**READY**                          |
| **2.2 kΩ** | **0.90 V**   | **184** | **63 °C**       | READY, thanh nhiệt vẫn bật —**nên dùng để test** |
| 1.5 kΩ           | 0.65 V             | 133           | 75 °C                 | READY, thanh nhiệt vẫn bật                                  |
| **1.2 kΩ** | **0.54 V**   | **110** | **82 °C**       | điểm thanh nhiệt**bật lại** (setpoint − 3 °C)     |
| **1.0 kΩ** | **0.45 V**   | **93**  | **88 °C**       | trên 85 °C → thanh nhiệt**tắt**                     |

Cặp **1.2 kΩ và 1.0 kΩ** đáng thử nhất: đổi qua đổi lại giữa hai con này là thấy chữ `H`
trong log bật/tắt đúng vùng chết 82–85 °C, đồng thời kiểm tra luôn ràng buộc min ON/OFF 5 giây.

### 5.3. Test bàn — biến trở 10 kΩ (chỉnh được liên tục)

Biến trở đã tự nó là mạch phân áp rồi, **không cần Rs 10k nữa**:

```
                 biến trở 10 kΩ
              ┌──────────────────┐
   +5V ───────┤ chân 1           │
              │                  │
              │   ◄══════════════╡ chân 2 (con chạy) ────► A0
              │                  │
   GND ───────┤ chân 3           │
              └──────────────────┘
```

Vặn con chạy về phía GND → ADC nhỏ → máy tưởng đang nóng.
Vặn về phía 5V → ADC lớn → máy tưởng đang lạnh.

Cách này tiện nhất: vừa vặn vừa nhìn Serial Monitor, thấy đúng khoảnh khắc `state=2`
nhảy sang `state=3` khi qua 55 °C.

### 5.4. Hai vùng cấm

```
   ADC ≥ 1015  →  temp = -32768  (INVALID)   ← A0 nối thẳng 5V, hoặc NTC đứt dây
   ADC ≤    8  →  temp = -32768  (INVALID)   ← A0 nối thẳng GND, hoặc NTC chập
```

Đừng cắm A0 thẳng vào 5V hay GND để thử — máy sẽ báo mất cảm biến và **cắt thanh nhiệt
ngay** (đúng thiết kế, `src/Controller.cpp`). Nhưng cũng chính vì vậy, rút một đầu điện
trở ra là cách nhanh nhất để test tình huống đứt dây NTC.

**Để hở chân A0 là sai cách test**: ADC đọc số rác, nhiệt độ nhảy loạn, thanh nhiệt đóng
cắt lung tung. Luôn phải có mạch phân áp ở A0.

---

## 6. Khi sửa code

`src/` ở đây là **bản copy** của `../source_code/src/`. Sửa ở bản gốc rồi đồng bộ lại:

```bash
cp ../source_code/src/*.h ../source_code/src/*.cpp src/
```

Bản gốc còn có 12 unit test + mô phỏng chạy trên PC (`../source_code/tools/sim/run.sh`),
Arduino IDE không chạy được phần đó.
