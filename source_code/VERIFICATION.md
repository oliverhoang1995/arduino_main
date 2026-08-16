# Verification Report

Chạy lại toàn bộ bằng 1 lệnh (không cần board, không cần PlatformIO):

```bash
./tools/sim/run.sh
```

Ngày chạy: 2026-08-16 · Compiler: Apple clang 14 · `-std=gnu++17 -Wall -Wextra -Wshadow` · **0 warning**

---

## 1. Unit test — 12/12 pass

Chạy trên PC vì `Controller.cpp` không phụ thuộc `Arduino.h`.

| # | Ca test | Kiểm điều gì |
|---|---|---|
| 1 | Phao thiếu 2999 ms → chưa công nhận; 3000 ms → công nhận. Chiều ngược lại 500 ms | Mốc chốt cứng của requirement01 §8 |
| 2 | Phao rung 2 Hz suốt 10 s | Không bao giờ công nhận sai |
| 3 | Vùng chết thanh nhiệt: 819→ON, 850→OFF, 830 (đang OFF)→vẫn OFF | Chống đóng cắt contactor liên tục |
| 4 | Phao Boiler báo thiếu | Cắt thanh nhiệt **ngay**, không chờ min-on 5 s |
| 5 | Không đọc được NTC | Không đun |
| 6 | Đang FILL, nước lạnh | Chưa gia nhiệt (requirement01 §4); van Tank mở, van Boiler đóng |
| 7 | Chu trình 65 s / 5 s / 12 s, FINISH → mở cửa → đóng cửa → chu trình mới | requirement01 §6, §7 |
| 8 | Thiếu nước + nhiệt độ tụt **giữa lúc đang rửa** | Van mở, thanh nhiệt bật, **bơm vẫn chạy, chu trình không dừng** (requirement01 §8) |
| 9 | Chưa đạt 55 °C dù cửa đóng | Không rửa; tụt dưới 54 °C ở READY → quay về HEAT |
| 10 | Mở cửa giữa lúc RINSE | Tắt bơm ngay, về READY; đóng lại → chu trình **mới từ đầu** (65 s) |
| 11 | Bấm POWER giữa chu trình | Tắt cả 5 output, về STANDBY; bấm lần nữa → chạy lại |
| 12 | `millis()` tràn giữa chu trình (49.7 ngày) | Toàn bộ chu trình vẫn đúng |

## 2. Số liệu đo được

| Hạng mục | Yêu cầu | Đo được | |
|---|---|---|---|
| WASH | 65 000 ms | **65 000 ms** | ✅ |
| WAIT | 5 000 ms | **5 000 ms** | ✅ |
| RINSE | 12 000 ms | **12 000 ms** | ✅ |
| Đóng cửa → bật bơm rửa | — | 120 ms (debounce 100 + quét 20) | ✅ |
| **Mở cửa → tắt bơm** | ≤ 150 ms | **80 ms** | ✅ |
| Phao báo thiếu → mở van | 3 000 ms | 3 020 ms (+1 chu kỳ quét) | ✅ |
| Phao báo đủ → đóng van | 500 ms | 520 ms | ✅ |
| Rung phao 2 Hz trong 30 s | không được mở van | mở 0 lần | ✅ |
| Sai số NTC 20–85 °C | ≤ ±2 °C | **≤ 0.5 °C** | ✅ |
| Xung nhiễu ADC đơn lẻ (~5 °C) | phải bị lọc | lệch 0.0 °C | ✅ |
| NTC đứt dây / ngắn mạch | không đun | thanh nhiệt OFF | ✅ |

## 3. Mô phỏng end-to-end

Gọi trực tiếp `setup()` / `loop()` của firmware thật với chân và đồng hồ giả, mô phỏng cả quá trình
gia nhiệt (nhiệt độ đổi theo trạng thái thanh nhiệt → ADC → NTC):

```
POWER → 2 van mở → Boiler đầy (van đóng sau 0.5 s) → Tank đầy → thanh nhiệt ON
     → 55 °C nhưng cửa mở: không rửa, tiếp tục hâm → 85 °C, thanh nhiệt cắt
     → cửa đóng → WASH 65 s → WAIT 5 s → RINSE 12 s → FINISH
     → phao rung 1 s: van không mở
     → mở cửa → READY → đóng cửa → chu trình 2
     → thiếu nước Tank 3 s giữa lúc rửa: van mở, bơm vẫn chạy
     → mở cửa giữa lúc rửa: bơm tắt, về READY
     → NTC đứt dây: thanh nhiệt OFF
```

Quan sát thêm từ log: trong RINSE nhiệt độ Boiler tụt xuống 81.9 °C → thanh nhiệt **tự bật lại trong
lúc bơm tráng vẫn chạy**, đúng requirement01 §8. Min ON 5 s của contactor cũng hoạt động đúng
(thanh nhiệt giữ ON thêm 5 s sau khi chạm 85 °C, quá nhiệt ~1 °C — trên máy thật thanh nhiệt tăng
chậm hơn nhiều nên quá nhiệt sẽ không đáng kể).

---

## 4. Chưa verify được trên máy này

Không có toolchain AVR và không có board, nên **những mục sau phải làm khi có bench** (`plans_02/03` §3):

| Hạng mục | Cách kiểm |
|---|---|
| Dung lượng Flash / SRAM thật | `pio run -e mega2560 -t size` |
| Thời gian vòng lặp thật trên AVR 16 MHz | Nhấp chân D13 mỗi vòng, xem oscilloscope |
| LCD thật: địa chỉ I2C, không nhấp nháy, tốc độ 400 kHz | Ca test bench 3, 4 |
| Relay không nhấp một nhịp lúc cấp nguồn | Ca test bench 1 (quan sát LED) |
| Chống nhiễu khi contactor đóng cắt thật | Ca test máy thật 8 |
| Chiều bơm, đấu đúng kênh relay | Ca test máy thật 1 — làm **trước khi** cấp nước |

Riêng phép `static_assert(config::kNtcPin == A0)` trong `Io.cpp` sẽ tự kiểm tra pin A0 lúc build cho AVR.

---

## 5. Ba điểm cần anh xác nhận

| # | Nội dung | Ghi chú |
|---|---|---|
| 1 | **Thanh nhiệt không chạy khi không đọc được NTC** (ADC ≤ 8 hoặc ≥ 1015 = ngắn/đứt dây) | Đây là 3 dòng code, **ngoài** danh sách chốt ở `plans_02/02-design.md` §9. Lý do: NTC đứt dây thì ADC = 1023, quy đổi ra nhiệt độ rất thấp, thanh nhiệt sẽ đun mãi. Nếu anh muốn bỏ: xoá điều kiện `in.tempDeci != kTempInvalid` trong `Controller::updateHeater()` |
| 2 | **Min ON / min OFF 5 s của thanh nhiệt** | Bảo vệ contactor. Đổi hoặc bỏ trong `Config.h` |
| 3 | **Mỗi chu trình đều phải đủ 55 °C mới chạy** | Theo requirement01 §6. Hệ quả: chu trình sau có thể phải chờ Boiler hâm lại vài chục giây — cần nói trước với khách hàng |

---

## 6. Ghi chú kỹ thuật

- Không có `new` / `malloc` / `String` trong toàn bộ `src/`.
- Không có `delay()` trong vòng lặp. Chỗ duy nhất dùng `delay()` là `LcdI2c::begin()`
  (~60 ms, chạy một lần trong `setup()`) — chuỗi khởi tạo bắt buộc theo datasheet HD44780.
- Ghi LCD là thao tác chặn: mỗi dòng 16 ký tự tốn ~4 ms trên I2C 400 kHz, và chỉ xảy ra
  khi nội dung dòng đó thay đổi (nhiều nhất 250 ms/lần). Chu kỳ điều khiển 20 ms có thể
  trễ tối đa ~9 ms trong nhịp đó — vẫn còn rất xa ngưỡng 150 ms của yêu cầu tắt bơm khi mở cửa
  (đo được 80 ms). **Chưa đo trên board thật.**
- Mọi phép so thời gian dùng `(uint32_t)(now - last) >= period` → đúng khi `millis()` tràn (test #12).
- `tools/sim/stub/` là bản giả của `Arduino.h` / `Wire.h` / `unity.h`, **chỉ dùng để verify
  trên PC**. Firmware thật không đụng tới thư mục này. Xoá `tools/` cũng không ảnh hưởng gì tới build.
