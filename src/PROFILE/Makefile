include ../Makefile.inc

CXXFLAGS+=-std=c++11
ifdef DEBUG
CXXFLAGS+=-Wall -Werror -Wextra -g
endif

profile: all

############################## Time Profiler #################################
BIN=CARMprof
BIN_LDFLAGS=-lunwind-ptrace -lunwind-generic
SRC=	Profile.cpp\
	Timer.cpp\
	ThreadTimer.cpp\
	../PINTOOL/Block.cpp\
	../PINTOOL/Function.cpp\
	../PINTOOL/Thread.cpp\
	../PINTOOL/Results.cpp

OBJ=$(patsubst %.cpp, %.o, $(SRC))

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(BIN_LDFLAGS)

TARGET=$(BIN)

############################## Hardware counters  ############################
ifdef USE_PAPI

LIB=libCARMprof.so
LIB_LDFLAGS+=-lhwloc
CFLAGS+=-D_PAPI_
LIB_LDFLAGS+=-lpapi
OBJ+=Sample.o

Sample.o: Sample.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -fPIC
$(LIB): Sample.o
	$(CXX) $(CXXFLAGS) -fPIC -shared $^ -o $@ $(LIB_LDFLAGS)

TARGET+=$(LIB)
endif

################################# all #######################################

all: $(TARGET)

clean:
	rm -rf $(OBJ) $(TARGET)

ifdef LIB
install: $(LIB) $(BIN)
	cp $(LIB) $(LIB_DIR)
	cp $(BIN) $(BIN_DIR)
uninstall:
	rm -f $(LIB_DIR)/$(LIB)
else
install: $(BIN)
	cp $(BIN) $(BIN_DIR)
uninstall:
	rm -f $(BIN_DIR)/$(BIN)
endif
