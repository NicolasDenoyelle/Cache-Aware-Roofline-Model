include ../Makefile.inc

.PHONY: all $(OBJ)

CPP=Block.cpp Function.cpp Thread.cpp Results.cpp
OBJ=$(patsubst %.cpp, %.o, $(CPP))

CONFIG_ROOT=$(PIN_ROOT)/source/tools/Config
include $(CONFIG_ROOT)/makefile.config

TOOL_CXXFLAGS += -std=c++11 -Wall -Wno-error
TOOL_LDFLAGS +=-Wl,-rpath=$(PIN_ROOT)/intel64/runtime -pthread
TOOL_LIBS+=-pthread
TOOL_ROOTS := CARMTool

ifdef DEBUG
TOOL_CXXFLAGS += -g -O0
else
TOOL_CXXFLAGS += -O2
endif

include $(TOOLS_ROOT)/Config/makefile.default.rules

install:
	cp obj-intel64/CARMTool.so $(LIB_DIR)/

uninstall:
	rm -f $(LIB_DIR)/CARMTool.so

clean:
	rm -rf obj-intel64 $(OBJ)

