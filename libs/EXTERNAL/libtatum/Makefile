#This is a simple wrapper hiding cmake from non-expert end users.
#
# It supports the targets:
#   'make'           - builds everything (all libaries/executables)
#   'make clean'     - removes generated build objects/libraries/executables etc.
#   'make distclean' - will clean everything including the cmake generated build files
#
# All other targets (e.g. 'make tatum_test') are passed to the cmake generated makefile
# and processed according to the CMakeLists.txt.
#
# To perform a debug build use:
#   'make BUILD_TYPE=debug'

#Default build type
# Possible values:
#    release
#    debug
BUILD_TYPE = release

#Allows users to pass parameters to cmake
#  e.g. make CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=true"
override CMAKE_PARAMS := -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -G 'Unix Makefiles' ${CMAKE_PARAMS}


# -s : Suppresss makefile output (e.g. entering/leaving directories)
# --output-sync target : For parallel compilation ensure output for each target is synchronized (make version >= 4.0)
MAKEFLAGS := -s

BUILD_DIR=./build
GENERATED_MAKEFILE := $(BUILD_DIR)/Makefile

#Check for the cmake exectuable
CMAKE := $(shell command -v cmake 2> /dev/null)

#Show test log on failures with 'make test'
export CTEST_OUTPUT_ON_FAILURE=TRUE

#All targets in this make file are always out of date.
# This ensures that any make target requests are forwarded to
# the generated makefile
.PHONY: all distclean $(GENERATED_MAKEFILE) $(MAKECMDGOALS)

#Build everything
all: $(GENERATED_MAKEFILE)
	@+$(MAKE) -C $(BUILD_DIR)

#Call the generated Makefile's clean, and then remove all cmake generated files
distclean: $(GENERATED_MAKEFILE)
	@ echo "Cleaning build..."
	@+$(MAKE) -C $(BUILD_DIR) clean 
	@ echo "Removing build system files.."
	@ rm -rf $(BUILD_DIR)
	@ rm -rf CMakeFiles CMakeCache.txt #In case 'cmake .' was run in the source directory

#Call cmake to generate the main Makefile
$(GENERATED_MAKEFILE):
ifeq ($(CMAKE),)
	$(error Required 'cmake' executable not found. On debian/ubuntu try 'sudo apt-get install cmake' to install)
endif
	@ mkdir -p $(BUILD_DIR)
	echo "cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) .. "
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) .. 

#Forward any targets that are not named 'distclean' to the generated Makefile
ifneq ($(MAKECMDGOALS),distclean)
$(MAKECMDGOALS): $(GENERATED_MAKEFILE)
	@+$(MAKE) -C $(BUILD_DIR) $(MAKECMDGOALS)
endif
