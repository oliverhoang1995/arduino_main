// May rua chen thuong mai - Arduino Mega 2560
// ============================================
//
// FILE NAY CO Y DE TRONG - dung xoa, cung dung viet code vao day.
//
// Arduino IDE bat buoc moi sketch phai co mot file .ino trung ten voi thu muc.
// Toan bo chuong trinh nam trong thu muc src/ va duoc IDE tu dong bien dich
// (Arduino IDE bien dich de quy thu muc con ten "src"):
//
//     src/main.cpp        <- setup() va loop() nam o day
//     src/Config.h        <- TOAN BO pin + tham so, doi phan cung chi sua file nay
//     src/Controller.*    <- logic dieu khien (state machine)
//     src/Io.*            <- doc 4 input, ghi 5 output relay
//     src/Ntc.*           <- doc NTC 10K -> nhiet do
//     src/Ui.* LcdI2c.*   <- LCD 16x2 I2C
//
// TRUOC KHI NAP
// -------------
//   Tools -> Board       : Arduino Mega or Mega 2560
//   Tools -> Processor   : ATmega2560 (Mega 2560)
//   Tools -> Port        : cong COM/tty cua board
//   Serial Monitor       : 115200 baud
//
// THU VIEN: khong can cai gi them. Chi dung <Wire.h> di kem Arduino IDE.
//
// Chon nham board (vi du Uno) se bao loi bien dich ngay tai src/Io.cpp
// ("kNtcPin phai la chan A0 cua Mega 2560") chu khong nap nham xuong may.
