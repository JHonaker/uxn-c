#!/usr/bin/env bash

FILE="./etc/test_roms/opctest.tal"
ROM="./etc/debug.rom"

make debug
./etc/uxnasm "$FILE" "$ROM"
./build/uxn "$ROM" -s 3