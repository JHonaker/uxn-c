#!/usr/bin/env bash

FILE="$1"
ROM="./etc/debug.rom"

make debug
./etc/uxnasm "$FILE" "$ROM"
./build/uxn "$ROM" -s 3