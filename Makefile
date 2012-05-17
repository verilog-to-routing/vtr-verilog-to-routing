####################################################################
# Makefile to build CAD tools in Verilog-to-Routing (VTR) Framework
####################################################################

all: notifications ODIN_II/odin_II.exe abc_with_bb_support/abc vpr/vpr
	
notifications: 
# checks if required packages are installed, and notifies the user if not
	if ! dpkg -l | grep exuberant-ctags -c >>/dev/null; then echo "\n\n\n\n********************************************************\n* Required package ctags not found.                    *\n* Use sudo apt-get install exuberant-ctags to install. *\n********************************************************\n\n\n\n"; fi
	if ! dpkg -l | grep bison -c >>/dev/null; then echo "\n\n\n\n**********************************************\n* Required package bison not found.          *\n* Use sudo apt-get install bison to install. *\n**********************************************\n\n\n\n"; fi
	if ! dpkg -l | grep flex -c >>/dev/null; then echo "\n\n\n\n*********************************************\n* Required package flex not found.          *\n* Use sudo apt-get install flex to install. *\n*********************************************\n\n\n\n"; fi
	if ! dpkg -l | grep g++ -c >>/dev/null; then echo "\n\n\n\n********************************************\n* Required package g++ not found.          *\n* Use sudo apt-get install g++ to install. *\n********************************************\n\n\n\n"; fi

packages:
# checks if required packages are installed, and installs them if not
	if ! dpkg -l | grep exuberant-ctags -c >>/dev/null; then sudo apt-get install exuberant-ctags; fi
	if ! dpkg -l | grep bison -c >>/dev/null; then sudo apt-get install bison; fi
	if ! dpkg -l | grep flex -c >>/dev/null; then sudo apt-get install flex; fi
	if ! dpkg -l | grep g++ -c >>/dev/null; then sudo apt-get install g++; fi
	cd vpr && make packages

.PHONY: packages

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

