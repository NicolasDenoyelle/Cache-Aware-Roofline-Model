include Makefile.inc

TARGET=benchmark profile
INSTALL=benchmark_install profile_install
UNINSTALL=benchmark_uninstall profile_uninstall
CLEAN=benchmark_clean profile_clean

ifdef PIN_ROOT
TARGET+=pintool
INSTALL+=pintool_install
UNINSTALL+=pintool_uninstall
CLEAN+=pintool_clean
endif

all: $(TARGET)

build_dirs: $(RES_DIR) $(BIN_DIR) $(LIB_DIR) $(HEADER_DIR)

install: $(TARGET) build_dirs $(INSTALL)
	@cp SCRIPTS/CARMPlot.R $(BIN_DIR)
	chmod +x $(BIN_DIR)/CARMPlot.R
	@cp SCRIPTS/CARMModel.sh $(BIN_DIR)
	sed -i "s/BIN_DIR/$(shell echo $(BIN_DIR) | sed -e 's/[\/]/\\\//g')/" $(BIN_DIR)/CARMModel.sh
	sed -i "s/LIB_DIR/$(shell echo $(LIB_DIR) | sed -e 's/[\/]/\\\//g')/" $(BIN_DIR)/CARMModel.sh
	sed -i "s/RES_DIR/$(shell echo $(RES_DIR) | sed -e 's/[\/]/\\\//g')/" $(BIN_DIR)/CARMModel.sh
	sed -i "s/PIN_ROOT/$(shell echo $(PIN_ROOT) | sed -e 's/[\/]/\\\//g')/" $(BIN_DIR)/CARMModel.sh
	chmod +x $(BIN_DIR)/CARMModel.sh

uninstall: $(UNINSTALL)

clean: $(CLEAN)

$(RES_DIR): 
	@mkdir $@

$(BIN_DIR): 
	@mkdir $@

$(LIB_DIR): 
	@mkdir $@

$(HEADER_DIR): 
	@mkdir $@

benchmark:
	make -C BENCHMARK
benchmark_install:
	make -C BENCHMARK install
benchmark_uninstall:
	make -C BENCHMARK uninstall
benchmark_clean:
	make -C BENCHMARK clean

profile:
	make -C PROFILE
profile_install:
	make -C PROFILE install
profile_uninstall:
	make -C PROFILE uninstall
profile_clean:
	make -C PROFILE clean

pintool:
	make -C PINTOOL
pintool_install:
	make -C PINTOOL install
pintool_uninstall:
	make -C PINTOOL uninstall
pintool_clean:
	make -C PINTOOL clean


