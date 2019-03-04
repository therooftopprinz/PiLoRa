#!/bin/sh
cd ..
mkdir build
cd build
../configure.py
make binstub -j4
objcopy -O binary --only-section=.rodata binstub binstub.rodata
mkdir -p tx && cd tx
rm log.bin
touch log.bin
../binstub --channel=1 --cx=127.0.0.1:2001 --tx=127.0.0.1:3001 --carrier=433000000 --bandwidth=500 --coding-rate=4/5 --spreading-factor=SF7 --reset-pin=33 --txrx-done-pin=32 &
~/Development/Logless/test/spawner ../binstub.rodata log.bin