####################################################################
# Makefile to build CAD tools in Verilog-to-Routing (VTR) Framework
####################################################################

all: ODIN_II/odin_II.exe abc_with_bb_support/abc vpr/vpr

ODIN_II/odin_II.exe: libvpr/libvpr_6.a
	cd ODIN_II && make

vpr/vpr: libvpr/libvpr_6.a
	cd vpr && make

abc_with_bb_support/abc:
	cd abc_with_bb_support && make

libvpr/libvpr_6.a:
	cd libvpr && make

clean:
	cd ODIN_II && make clean
	cd abc_with_bb_support && make clean
	cd vpr && make clean
	cd libvpr && make clean

