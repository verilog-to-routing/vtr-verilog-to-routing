#####################################################################
# Makefile to build CAD tools in Verilog-to-Routing (VTR) Framework #
#####################################################################

SUBDIRS = abc_with_bb_support ODIN_II vpr libarchfpga liblog ace2 libsdc_parse

all: notifications subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@ $(MAKE) -C $@ --no-print-directory VERBOSITY=0
	
notifications: 
# checks if required packages are installed, and notifies the user if not
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep exuberant-ctags -c >>/dev/null; then echo "\n\n\n\n***************************************************************\n* Required package 'ctags' not found.                         *\n* Type 'make packages' to install all packages, or            *\n* 'sudo apt-get install exuberant-ctags' to install manually. *\n***************************************************************\n\n\n\n"; fi; fi
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep bison -c >>/dev/null; then echo "\n\n\n\n*****************************************************\n* Required package 'bison' not found.               *\n* Type 'make packages' to install all packages, or  *\n* 'sudo apt-get install bison' to install manually. *\n*****************************************************\n\n\n\n"; fi; fi
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep flex -c >>/dev/null; then echo "\n\n\n\n*****************************************************\n* Required package 'flex' not found.                *\n* Type 'make packages' to install all packages, or  *\n* 'sudo apt-get install flex' to install manually.  *\n*****************************************************\n\n\n\n"; fi; fi
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep g++ -c >>/dev/null; then echo "\n\n\n\n*****************************************************\n* Required package 'g++' not found.                 * \n* Type 'make packages' to install all packages, or  *\n* 'sudo apt-get install g++' to install manually.   *\n*****************************************************\n\n\n\n"; fi; fi

packages:
# checks if required packages are installed, and installs them if not
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep exuberant-ctags -c >>/dev/null; then sudo apt-get install exuberant-ctags; fi; fi
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep bison -c >>/dev/null; then sudo apt-get install bison; fi; fi
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep flex -c >>/dev/null; then sudo apt-get install flex; fi; fi
	@ if cat /etc/issue | grep Ubuntu -c >>/dev/null; then if ! dpkg -l | grep g++ -c >>/dev/null; then sudo apt-get install g++; fi; fi
	@ cd vpr && make packages

ODIN_II: libarchfpga

vpr: libarchfpga libsdc_parse

libarchfpga: liblog

ace2: abc_with_bb_support
clean:
	@ cd ODIN_II && make clean
	@ cd abc_with_bb_support && make clean
	@ cd ace2 && make clean
	@ cd vpr && make clean
	@ cd libarchfpga && make clean
	@ cd libsdc_parse && make clean
	@ cd liblog && make clean

clean_vpr:
	@ cd vpr && make clean

get_titan_benchmarks:
	@ echo "Warning: A typical Titan release is a ~1GB download, and uncompresses to ~10GB."
	@ echo "Starting download in 15 seconds..."
	@ sleep 15
	@ ./vtr_flow/scripts/download_titan.py --vtr_flow_dir ./vtr_flow
	@ echo "Titan architectures: vtr_flow/arch/titan"
	@ echo "Titan benchmarks: vtr_flow/benchmarks/titan_blif"

.PHONY: packages subdirs $(SUBDIRS)
