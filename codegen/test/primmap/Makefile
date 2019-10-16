.PHONY: clean sim

all:
	fletchgen -r twoprimread.rb -i twoprimwrite.as -s memory.srec -l dot vhdl --sim --regs c:8:add s:32:sum

sim:
	rm -f vhdl/Kernel.gen.vhd
	rm -f vhdl/SimTop_tc.gen.vhd
	vhdeps -i ${FLETCHER_DIR}/hardware -i . ghdl SimTop_tc

questa:
	rm -f vhdl/Kernel.gen.vhd
	rm -f vhdl/SimTop_tc.gen.vhd
	vhdeps -i ${FLETCHER_DIR}/hardware -i . vsim SimTop_tc

gui:
	rm -f vhdl/Kernel.gen.vhd
	rm -f vhdl/SimTop_tc.gen.vhd
	vhdeps -i ${FLETCHER_DIR}/hardware -i . --gui ghdl SimTop_tc

clean:
	rm -rf vhdmmio-doc
	rm -f fletchgen.mmio.yaml
	rm -f memory.srec
	rm -r dot/*.dot
	rm -f vhdl/*.gen.vhd
