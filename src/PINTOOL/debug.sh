#!/bin/sh

PIN_ROOT=$HOME/Documents/pin-3.0-76991-gcc-linux

$PIN_ROOT/pin -pause_tool 2 -t obj-intel64/CARMTool.so -- $* > pin.out &
sleep 1
cat pin.out
pid=$(awk /pid/'{print $11}' pin.out)
cmd=$(awk /add/'{print}' pin.out)

gdb --quiet \
    --eval-command="$cmd" \
    --pid $pid $PIN_ROOT/intel64/bin/pinbin

rm pin.out
