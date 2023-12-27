SRC := ./src
SOURCES := $(SRC)/core/f_permutation.v \
		   $(SRC)/core/keccak.v \
		   $(SRC)/core/padder.v \
		   $(SRC)/core/padder1.v \
		   $(SRC)/core/rconst.v \
		   $(SRC)/core/round.v \
		   $(SRC)/KetchupPeripheral_v1_0_S00_AXI.v

TESTBENCH := ./testbench/keccak_peripheral_tb.v

OUTFILE_NAME := simulation.vvp
WAVEFILE_NAME := signals.vcd

.PHONY: simulate
simulate: $(WAVEFILE_NAME)

waves: $(WAVEFILE_NAME)
	gtkwave $(WAVEFILE_NAME)

.PHONY: compile
compile: $(OUTFILE_NAME)

$(WAVEFILE_NAME): $(OUTFILE_NAME)
	vvp $(OUTFILE_NAME) $(VVP_FLAGS)

$(OUTFILE_NAME): $(SOURCES) $(TESTBENCH)
	iverilog -o $(OUTFILE_NAME) $(SOURCES) $(TESTBENCH)
