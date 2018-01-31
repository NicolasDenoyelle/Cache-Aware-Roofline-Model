#!/bin/sh

PIN_ROOT=$HOME/Documents/pin-3.0-76991-gcc-linux/pin
PIN_TOOL=$PWD/obj-intel64/CARMTool.so
cmd=$*
BIN=$( echo $1 | awk '{n=split($0, s, "/"); print s[n]}')
OUT=$BIN.out

echo "Run $BIN to get flops and bytes"
$PIN_ROOT -t $PIN_TOOL -AI $OUT -- $cmd 2>&1 1>/dev/null

echo "Run $BIN to get time"
$PIN_ROOT -t $PIN_TOOL -time $OUT -- $cmd 2>&1 1>/dev/null

echo "Output to $OUT"

