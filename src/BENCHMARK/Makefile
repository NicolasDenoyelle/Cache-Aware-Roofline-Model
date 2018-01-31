#############################################################################################
#                                      Configuration                                        #
#############################################################################################
include ../Makefile.inc

#############################################################################################
#                                      Plateform Benchmark                                  #
#############################################################################################
SRC=stream.c output.c types.c stats.c topology.c roofline.c list.c
OBJ=$(patsubst %.c, %.o, $(SRC)) MSC/$(MSC)/MSC.a
CFLAGS+=-DROOFLINE_N_SAMPLES=$(ROOFLINE_N_SAMPLES) -DCPU_FREQ=$(CPU_FREQUENCY) -g
LDFLAGS+=-ldl -lhwloc -lm $(OMP_FLAG)
BIN=roofline
LIB=lib$(BIN).so

ifdef DEBUG
CFLAGS+=-Wall -Werror -Wextra -g -DDEBUG2 -DDEBUG1
endif

all: $(BIN) $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(OMP_FLAG) -fPIC

#Benchmark for intel plateform. check MSC direcotry for other benchmark implementations.
MSC=intel
MSC/$(MSC)/MSC.a: MSC/$(MSC)/*.c
	make -C MSC/$(MSC)

$(BIN): main.c $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(LIB): $(OBJ)
	$(CC) $(CFLAGS) -fPIC -shared $^ -o $@ $(LDFLAGS)

install:
	cp $(BIN) $(BIN_DIR)/
	cp $(LIB) $(LIB_DIR)/
	cp stream.h output.h utils.h $(HEADER_DIR)/

uninstall:
	rm -f $(BIN_DIR)/$(BIN) $(LIB_DIR)/$(LIB) $(HEADER_DIR)/utils.h $(HEADER_DIR)/output.h $(HEADER_DIR)/stream.h

clean:
	rm -f $(OBJ) $(LIB) $(BIN)
	make -C MSC/$(MSC) clean

