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

| Thư viện  | Nguồn                            |
| --------- | -------------------------------- |
| `Wire.h`  | có sẵn trong Arduino IDE         |
| LCD I2C   | `src/LcdI2c.cpp` — tự viết trong dự án |

LCD 16×2 chạy qua module I2C PCF8574. Driver tự dò địa chỉ **0x27** rồi **0x3F**
(hai địa chỉ phổ biến nhất). Không thấy LCD thì máy **vẫn chạy bình thường**,
chỉ không hiển thị, và Serial in ra `LCD not found`.

Nếu LCD sáng đèn nhưng không ra chữ / ra ký tự rác:

| Hiện tượng                     | Sửa ở đâu                                                     |
| ------------------------------ | ------------------------------------------------------------- |
| Không thấy LCD (địa chỉ khác) | `src/Config.h` → `kLcdAddresses`                              |
| Ký tự rác, cáp I2C đi dài     | `src/Config.h` → `kI2cClockHz` hạ xuống `100000`              |
| Sáng đèn nhưng trắng trơn     | vặn biến trở tương phản trên lưng module                      |
| Module nối chân khác chuẩn     | 4 hằng số `kBitRs/kBitEn/kBitBacklight` đầu `src/LcdI2c.cpp`  |

---

## 4. Nối dây (tóm tắt)

Đầy đủ + 3 lưu ý điện bắt buộc: xem `../source_code/README.md` mục 3.

| Vào (INPUT_PULLUP, tiếp điểm về GND) | Pin |     | Ra (relay active-LOW) | Pin |
| ------------------------------------ | --- | --- | --------------------- | --- |
| Cảm biến cửa (ACTIVE = đóng)       | D22 |     | Bơm rửa              | D2  |
| Phao Tank (ACTIVE = thiếu nước)     | D23 |     | Bơm tráng            | D3  |
| Phao Boiler (ACTIVE = thiếu nước)   | D24 |     | Thanh nhiệt Boiler    | D4  |
| Nút POWER                           | D26 |     | Van cấp nước Tank    | D5  |
| NTC 10K                              | A0  |     | Van cấp nước Boiler  | D6  |
|                                      |     |     | LCD I2C SDA / SCL     | D20 / D21 |

Đổi pin, đổi tiếp điểm NO↔NC, đổi relay active-HIGH: **chỉ sửa `src/Config.h`**.

---

## 5. Khi sửa code

`src/` ở đây là **bản copy** của `../source_code/src/`. Sửa ở bản gốc rồi đồng bộ lại:

```bash
cp ../source_code/src/*.h ../source_code/src/*.cpp src/
```

Bản gốc còn có 12 unit test + mô phỏng chạy trên PC (`../source_code/tools/sim/run.sh`),
Arduino IDE không chạy được phần đó.
