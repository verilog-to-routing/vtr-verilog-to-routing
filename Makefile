#This is a simple wrapper which hides cmake (for convenience, and from non-expert end users).
#
# It supports the targets:
#   'make'           - builds everything (all libaries/executables)
#   'make clean'     - removes generated build objects/libraries/executables etc.
#   'make distclean' - will clean everything including the cmake generated build files
#
# All other targets (e.g. 'make vpr') are passed to the cmake generated makefile
# and processed according to the CMakeLists.txt.
#
# To perform a debug build use:
#   'make BUILD_TYPE=debug'

#Default build type
# Possible values:
#    release_pgo	#Perform a 2-stage build with profile-guided compiler optimization
#    release		#Build with compiler optimization
#    debug			#Build with debug info and no compiler optimization
BUILD_TYPE ?= release

#Convert to lower case for consistency
BUILD_TYPE := $(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]')

#Trim any _pgo in the build type name (since it would not match any of
#CMake's standard build types)
CMAKE_BUILD_TYPE := $(shell echo $(BUILD_TYPE) | sed 's/_\?pgo//')

#Allows users to pass parameters to cmake
#  e.g. make CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=true"
override CMAKE_PARAMS := -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G 'Unix Makefiles' ${CMAKE_PARAMS}


# -s : Suppresss makefile output (e.g. entering/leaving directories)
# --output-sync target : For parallel compilation ensure output for each target is synchronized (make version >= 4.0)
MAKEFLAGS := -s

SOURCE_DIR := $(PWD)
BUILD_DIR ?= build

#Check for the cmake exectuable
CMAKE := $(shell command -v cmake 2> /dev/null)

#Show test log on failures with 'make test'
export CTEST_OUTPUT_ON_FAILURE=TRUE

#All targets in this make file are always out of date.
# This ensures that any make target requests are forwarded to
# the generated makefile
.PHONY: all distclean $(MAKECMDGOALS)

#For an 'all' build with BUILD_TYPE containing 'pgo' this will perform a 2-stage compilation
#with profile guided optimization.
#For a BUILD_TYPE without 'pgo', a single stage (non-pgo) compilation is performed.

#Forward any targets that are not named 'distclean' or 'clean' to the generated Makefile
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
all $(MAKECMDGOALS):
ifneq ($(BUILD_DIR),build)
	ln -sf $(BUILD_DIR) build
endif
ifeq ($(CMAKE),)
	$(error Required 'cmake' executable not found. On debian/ubuntu try 'sudo apt-get install cmake' to install)
endif
	@ mkdir -p $(BUILD_DIR)
ifneq (,$(findstring pgo,$(BUILD_TYPE)))
	#
	#Profile Guided Optimization Build
	#
	@echo "Performing Profile Guided Optimization (PGO) build..."
	#
	#1st-stage build for profile generation
	#
	echo "cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) -DVPR_PGO_CONFIG=prof_gen $(SOURCE_DIR)"
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) -DVPR_PGO_CONFIG=prof_gen $(SOURCE_DIR)
	@+$(MAKE) -C $(BUILD_DIR) $(MAKECMDGOALS)
	#
	#Run benchmarks to generate profiling data
	#
	echo "Generating profile data for PGO (may take several minutes)"
	#Need titan benchmarks for pgo_profile task
	@+$(MAKE) -C $(BUILD_DIR) get_titan_benchmarks
	#Note profiling must be done serially to avoid corrupting the generated profiles
	./run_reg_test.pl pgo_profile
	#
	#Configure 2nd-stage build to use profiling data to guide compiler optimization
	#
	echo "cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) -DVPR_PGO_CONFIG=prof_use $(SOURCE_DIR)"
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) -DVPR_PGO_CONFIG=prof_use $(SOURCE_DIR)
else #BUILD_TYPE not containing 'pgo'
	#
	#Configure for standard build
	#
	@echo "Performing standard build..."
	echo "cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) $(SOURCE_DIR)"
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) $(SOURCE_DIR)
endif #BUILD_TYPE
	#
	#Final build
	#
	@echo "Building target(s): $(MAKECMDGOALS)"
	@+$(MAKE) -C $(BUILD_DIR) $(MAKECMDGOALS)
endif #clean
endif #distclean

#Call the generated Makefile's clean, and then remove all cmake generated files
distclean: clean
	@ echo "Removing build system files.."
	@ rm -rf $(BUILD_DIR)
	@ rm -rf CMakeFiles CMakeCache.txt #In case 'cmake .' was run in the source directory

clean:
ifeq ($(CMAKE),)
	$(error Required 'cmake' executable not found. On debian/ubuntu try 'sudo apt-get install cmake' to install)
endif
	@ echo "Cleaning files.."
	#We run cmake so we can use the generated Makefile to clean any executables
	#generated outside the build directory
	@ mkdir -p $(BUILD_DIR)
	echo "cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) $(SOURCE_DIR)"
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) $(SOURCE_DIR)
	@+$(MAKE) -C $(BUILD_DIR) clean

