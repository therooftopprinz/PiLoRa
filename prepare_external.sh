#!/bin/sh

git submodule update --init --recursive

pushd . && cd PiGpioHwApi && mkdir -p build && cd build && ../configure.py && make pigpiohwapi.a && popd
pushd . && cd Logless && mkdir -p build && cd build && ../configure.py && make logless.a && make spawner && popd