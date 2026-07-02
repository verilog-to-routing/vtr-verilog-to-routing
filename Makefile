# This is a simple wrapper which hides cmake (for convenience, and from non-expert end users).
#
# It supports the targets:
#   'make'           - builds everything (all libraries/executables)
#   'make clean'     - removes generated build objects/libraries/executables etc.
#   'make distclean' - will clean everything including the cmake generated build files
#
# All other targets (e.g. 'make vpr') are passed to the cmake generated makefile
# and processed according to the CMakeLists.txt.
#
# To perform a debug build use:
#   'make BUILD_TYPE=debug'
#
# To enable debugging verbose messages as well use:
#
#   'make BUILD_TYPE=debug VERBOSE=1'

# Build type
# Possible values (not case sensitive):
#    release            #Build with compiler optimization (Default)
#    RelWithDebInfo     #Build with debug info and compiler optimizations
#    debug              #Build with debug info and no compiler optimization
# Possible suffixes:
#    _pgo               #Perform a 2-stage build with profile-guided compiler optimization
#    _strict            #Build VPR with warnings treated as errors
BUILD_TYPE ?= release

#Debugging verbosity enable
VERBOSE ?= 0

#Convert to lower case for consistency
BUILD_TYPE := $(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]')

#Trim any _pgo or _strict in the build type name (since it would not match any of
#CMake's standard build types)
CMAKE_BUILD_TYPE := $(shell echo $(BUILD_TYPE) | sed 's/_\?pgo//' | sed 's/_\?strict//')

#Detect the operating system
UNAME_S := $(shell uname -s)

# Cmake generator has to be Ninja on MSVC
CMAKE_GEN = Unix Makefiles
ifeq ($(OS),Windows_NT)
CMAKE_GEN = Ninja
# Msys2 can still use Linux gcc
ifeq ($(MSYSTEM),MINGW64)
CMAKE_GEN = Unix Makefiles
endif
endif

#Allows users to pass parameters to cmake
#  e.g. make CMAKE_PARAMS="-DVTR_ENABLE_SANITIZE=true"
override CMAKE_PARAMS := -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G '${CMAKE_GEN}' ${CMAKE_PARAMS}

# The curl path should be defined by user. Try to get one from system
CURL_PATH:=$(shell where curl.exe 2>nul | head -n 1)
# VCPKG root is a system variable env:VCPKG in power shell. User can override by using VCPKG_PATH when calling the makefile
VCPKG_PATH:=$(subst \,/,$(VCPKG_ROOT))
ifeq ($(OS),Windows_NT)
# Msys2 can still use Linux gcc
ifeq ($(MSYSTEM),MINGW64)
	override CMAKE_PARAMS := ${CMAKE_PARAMS} -DWITH_PARMYS=OFF -DSLANG_SYSTEMVERILOG=OFF -DVTR_IPO_BUILD=OFF -DWITH_ABC=OFF
else
	override CMAKE_PARAMS := ${CMAKE_PARAMS} -DWGET=${CURL_PATH} -DCMAKE_TOOLCHAIN_FILE="${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake"
	override CMAKE_PARAMS := ${CMAKE_PARAMS} -DWITH_PARMYS=OFF -DSLANG_SYSTEMVERILOG=OFF -DVTR_IPO_BUILD=OFF -DWITH_ABC=OFF
endif
endif

#Are we doing a strict (i.e. warnings as errors) build?
ifneq (,$(findstring strict,$(BUILD_TYPE)))
	#Configure for strict build with VPR warning treated as errors
override CMAKE_PARAMS := -DCMAKE_COMPILE_WARNING_AS_ERROR=on ${CMAKE_PARAMS}
endif #Strict build type

#Enable verbosity
ifeq ($(VERBOSE),1)
override CMAKE_PARAMS := -DVTR_ENABLE_VERBOSE=on ${CMAKE_PARAMS}
endif

#`make ensure-gui` / `make ensure-headless` set VTR_GRAPHICS to force the
#graphics option. They delegate to the normal build rule below, so the PGO /
#build-dir / cmake handling is inherited rather than duplicated.
ifeq ($(VTR_GRAPHICS),on)
override CMAKE_PARAMS := ${CMAKE_PARAMS} -DVPR_USE_EZGL=on
endif
ifeq ($(VTR_GRAPHICS),off)
override CMAKE_PARAMS := ${CMAKE_PARAMS} -DVPR_USE_EZGL=off
endif

# -s : Suppresses makefile output (e.g. entering/leaving directories)
# --output-sync target : For parallel compilation ensure output for each target is synchronized (make version >= 4.0)
MAKEFLAGS := -s

ifeq ($(OS),Windows_NT)
ifeq ($(MSYSTEM),MINGW64)
SOURCE_DIR := $(PWD)
else
SOURCE_DIR := $(shell powershell -NoProfile -Command "(Get-Location).Path")
SOURCE_DIR := $(subst \,/,$(SOURCE_DIR))
endif
else
SOURCE_DIR := $(PWD)
endif
BUILD_DIR ?= build

#Check for the cmake executable
CMAKE := $(shell command -v cmake 2> /dev/null)
ifeq ($(OS),Windows_NT)
ifneq ($(MSYSTEM),MINGW64)
CMAKE := cmake.exe
endif
endif

#Show test log on failures with 'make test'
export CTEST_OUTPUT_ON_FAILURE=TRUE

#All targets in this make file are always out of date.
# This ensures that any make target requests are forwarded to
# the generated makefile
.PHONY: all distclean ensure-gui ensure-headless $(MAKECMDGOALS)

#For an 'all' build with BUILD_TYPE containing 'pgo' this will perform a 2-stage compilation
#with profile guided optimization.
#For a BUILD_TYPE without 'pgo', a single stage (non-pgo) compilation is performed.

#Forward any targets that are not named 'distclean', 'clean', 'ensure-gui' or 'ensure-headless' to the generated Makefile
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),ensure-gui)
ifneq ($(MAKECMDGOALS),ensure-headless)
all $(MAKECMDGOALS):
ifneq ($(BUILD_DIR),build)
	ln -sf $(BUILD_DIR) build
endif
ifeq ($(CMAKE),)
	ifeq ($(UNAME_S),Darwin)
		$(error Required 'cmake' executable not found. On macOS try 'brew install cmake' to install)
	else
		$(error Required 'cmake' executable not found. On debian/ubuntu try 'sudo apt-get install cmake' to install)
	endif
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
	./run_reg_test.py pgo_profile
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
endif #ensure-headless
endif #ensure-gui
endif #clean
endif #distclean

#Build VTR WITH graphics. Provision a Qt6 SDK (>= the supported floor) first,
#honoring ensure_qt6_sdk.sh's exit code (make aborts if it fails), then delegate
#to the normal build with graphics forced on. Defined after the 'all' rule so
#'all' stays the default goal.
#Builds the default 'all' goal (not just 'vpr') so the full flow -- synthesis
#(yosys + parmys), mapping (abc) and place-and-route (vpr) -- is produced. The
#yosys/parmys targets are 'ALL' custom targets that 'make vpr' would skip,
#leaving build/bin/yosys missing and run_vtr_flow unable to find yosys.
ensure-gui:
	@ $(SOURCE_DIR)/dev/ensure_qt6_sdk.sh
	@+$(MAKE) all VTR_GRAPHICS=on

#Build VTR WITHOUT graphics (headless); no Qt SDK is needed.
ensure-headless:
	@+$(MAKE) all VTR_GRAPHICS=off

#Call the generated Makefile's clean, and then remove all cmake generated files
distclean: clean
	@ echo "Removing build system files.."
	@ rm -rf $(BUILD_DIR)
	@ rm -rf CMakeFiles CMakeCache.txt #In case 'cmake .' was run in the source directory

clean:
ifeq ($(CMAKE),)
	ifeq ($(UNAME_S),Darwin)
		$(error Required 'cmake' executable not found. On macOS try 'brew install cmake' to install)
	else
		$(error Required 'cmake' executable not found. On debian/ubuntu try 'sudo apt-get install cmake' to install)
	endif
endif
	@ echo "Cleaning files.."
	#We run cmake so we can use the generated Makefile to clean any executables
	#generated outside the build directory
	@ mkdir -p $(BUILD_DIR)
	echo "cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) $(SOURCE_DIR)"
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_PARAMS) $(SOURCE_DIR)
	@+$(MAKE) -C $(BUILD_DIR) clean

