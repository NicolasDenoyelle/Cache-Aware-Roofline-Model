##Install configuration
PREFIX=/home/ndenoyel/Documents/LARM#                   INSTALL directory
RES_DIR=$(PREFIX)/.CARM#     Directory where to output results
LIB_DIR=$(PREFIX)/lib#       Directory for libraries and pintool
BIN_DIR=$(PREFIX)/bin#       Directory for plateform benchmark, profiler, and plot script.
HEADER_DIR=$(PREFIX)/include#Directory for headers

##Pin Root directory. Used to build instrumentation tool. Target application must be compiled with -g
PIN_ROOT=$(HOME)/Documents/pin-3.0-76991-gcc-linux

##Define whether you installed PAPI and want to compile the function instrumenration library with PAPI support.
#USE_PAPI=1

##Define the number of sample for each plateform benchmark.
ROOFLINE_N_SAMPLES=16

##Runtime Information. Used to output and find the plateform benchmark result.
HOST=$(shell hostname)

##Set the exact CPU frequency for better accuracy of plateform benchmark.
CPU_FREQUENCY=$(shell lscpu | awk /max/'{print $$NF*1000000}')

##Compiler informations
CC=gcc
CXX=g++
OMP_FLAG=-fopenmp
LDFLAGS=#Additional flags to find required libraries
CFLAGS=-I$(HOME)/local/include#Additional flags to find required headers
CXXFLAGS=-I$(HOME)/local/include#Additional flags to find required headers
#DEBUG=1 #Compile with verbose infos + debug symbols.

