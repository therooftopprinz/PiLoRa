#!/bin/sh
cd ..
mkdir build
cd build
../configure.py
make binstub -j
./binstub --channel=1 --tx=127.0.0.1:8899 --carrier=433000000 --bandwidth=500 --coding-rate=4/5 --spreading-factor=SF7 --reset-pin=33 --txrx-done-pin=32