#!/bin/sh

if [ -z $1 ]; then
    echo "$0 prog <prog_args ...>"
    exit
fi

#Benchmark system
BENCHMARK=BIN_DIR/roofline
BENCHMARK_OUTPUT=RES_DIR/$(hostname).roofs
echo "Benchmark system..."
$BENCHMARK -t "load|FMA" -o $BENCHMARK_OUTPUT >/dev/null
echo "Machine benchmark result in $BENCHMARK_OUTPUT"

#Profile Application time
PROFILE=BIN_DIR/CARMprof
PROFILE_OUTPUT=RES_DIR/$(basename $1).out
echo "Running application to get functions timing..."
$PROFILE -o $PROFILE_OUTPUT -- $* >/dev/null
echo "Application timings in $PROFILE_OUTPUT"

#Profile applcation flops and bytes
PIN=PIN_ROOT/pin
PIN_TOOL=LIB_DIR/CARMTool.so
echo "Running application to get flops and bytes..."
$PIN -t $PIN_TOOL -ifeellucky -input $PROFILE_OUTPUT -output $PROFILE_OUTPUT -- $* >/dev/null
echo "Application timings, flops and bytes in $PROFILE_OUTPUT"
#Clean logs
rm -f pintool.log
rm -f pin.log

#Plot results
echo "Plotting results..."
BIN_DIR/CARMPlot.R -i $BENCHMARK_OUTPUT -d $PROFILE_OUTPUT 

