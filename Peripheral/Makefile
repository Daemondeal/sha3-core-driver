SRC := ./src
SOURCES := $(SRC)/core/f_permutation.v \
		   $(SRC)/core/keccak.v \
		   $(SRC)/core/padder.v \
		   $(SRC)/core/padder1.v \
		   $(SRC)/core/rconst.v \
		   $(SRC)/core/round.v \
		   $(SRC)/KetchupPeripheral_v1_0_S00_AXI.v

TESTBENCH := ./testbench/keccak_peripheral_tb.v
TESTVECTORS := ./testvectors/512.mem \
			   ./testvectors/384.mem \
			   ./testvectors/256.mem \
			   ./testvectors/224.mem

OUTFILE_NAME    := simulation.vvp
WAVEFILE_NAME   := signals.vcd
TESTS_GENERATOR := maketests.py

.PHONY: simulate
simulate: $(WAVEFILE_NAME)

waves: $(WAVEFILE_NAME)
	gtkwave $(WAVEFILE_NAME)

.PHONY: compile
compile: $(OUTFILE_NAME)

$(WAVEFILE_NAME): $(OUTFILE_NAME) $(TESTVECTORS)
	@echo "### SIMULATING ###"
	vvp $(OUTFILE_NAME) $(VVP_FLAGS)
	@echo 

$(OUTFILE_NAME): $(SOURCES) $(TESTBENCH)
	@echo "### COMPILING ###"
	iverilog -o $(OUTFILE_NAME) $(SOURCES) $(TESTBENCH)
	@echo

$(TESTVECTORS): maketests.py
	@echo "### GENERATING TESTS ###"
	python $(TESTS_GENERATOR)
	@echo