SRC := ../Peripheral
SOURCES := $(SRC)/core/f_permutation.v $(SRC)/core/keccak.v $(SRC)/core/padder.v $(SRC)/core/padder1.v $(SRC)/core/rconst.v $(SRC)/core/round.v $(SRC)/SHA3Keccak_v1_0_S00_AXI.v

TESTBENCH := ./keccak_peripheral_tb.v

OUTFILE_NAME := simulation.vvp

all:
	iverilog -o $(OUTFILE_NAME) $(SOURCES) $(TESTBENCH)
	vvp $(OUTFILE_NAME)