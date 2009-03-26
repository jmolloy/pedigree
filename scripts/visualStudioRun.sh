#!/bin/bash

cd $1

# run QEMU
rm -f serial.txt && ../scripts/qemu -L "C:/qemu" -serial file:serial.txt -localtime -m 1024
