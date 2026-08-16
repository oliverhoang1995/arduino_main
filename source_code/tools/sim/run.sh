#!/bin/sh
# Chay toan bo phan verify tren PC: unit test + mo phong end-to-end + do so lieu.
# Khong can board, khong can PlatformIO - chi can mot trinh bien dich C++17.
#
#   ./tools/sim/run.sh
#
# stub/ la ban gia cua Arduino.h / Wire.h / unity.h, CHI dung cho may
# tinh. Firmware that khong dung den thu muc nay.

set -e
cd "$(dirname "$0")"

CXX=${CXX:-c++}
SRC=../../src
FLAGS="-std=gnu++17 -Wall -Wextra -Wshadow -I$SRC -Istub"
FIRMWARE="$SRC/main.cpp $SRC/Io.cpp $SRC/Ntc.cpp $SRC/Ui.cpp $SRC/LcdI2c.cpp $SRC/Controller.cpp"

mkdir -p build

echo "=============================================================="
echo " 1/3  UNIT TEST (logic dieu khien)"
echo "=============================================================="
$CXX $FLAGS $SRC/Controller.cpp ../../test/test_controller/test_main.cpp -o build/unit
./build/unit

echo
echo "=============================================================="
echo " 2/3  DO SO LIEU (NTC, tre tat bom, xac nhan phao)"
echo "=============================================================="
$CXX $FLAGS verify.cpp $FIRMWARE -o build/verify
./build/verify | grep -v '^[0-9]*,state'

echo
echo "=============================================================="
echo " 3/3  MO PHONG END-TO-END (toan bo luong requirement)"
echo "=============================================================="
$CXX $FLAGS sim.cpp $FIRMWARE -o build/sim
./build/sim
