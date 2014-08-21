################################################################################
# Build/Release Parameters													   #
################################################################################

# If ENABLE_GRAPHICS is set to true, will require X11 to compile.  
# On Ubuntu: 'sudo apt-get install libx11-dev' to install.  

ENABLE_GRAPHICS = true #'true' or 'false'

export BUILD_TYPE = release #'debug' or 'release'

OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD = -O3

################################################################################
# Input Files        														   #
################################################################################
.SUFFIXES: .o .cpp .c

SRC_DIR = src
OBJ_DIR = obj

exe = sta
test = $(exe)_test


#Main program (exe) sources
DIRS = $(SRC_DIR)/timing_graph $(SRC_DIR)/waveform_calc $(SRC_DIR)/util $(SRC_DIR)/base $(SRC_DIR)/version

# main must be kept seperate to avoid conflicts with main defined in unit tests
exe_main = $(SRC_DIR)/main.cpp 
exe_src_cpp = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
exe_src_c = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))

#Unit test sources
TEST_DIRS = $(SRC_DIR)/tests
test_src_cpp = $(foreach dir, $(TEST_DIRS), $(wildcard $(dir)/*.cpp))


################################################################################
# Inferred Parameters  														   #
################################################################################
BUILD_VERSION := `git describe --always --long --dirty=-modified && touch src/version/version.cpp`#Touch forces re-build which updated version and build time
BUILD_DATE := `date -R`

################################################################################
# Compilation Flags    														   #
################################################################################
CXX = g++

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings

DEBUG_FLAGS = -g -O0
TUNE_FLAGS = 
#Try to make the linker optimize accross object files. Experimental if used with -g
OPT_FLAGS = -g $(OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD) $(TUNE_FLAGS) -march=native #-fno-inline
PROFILE_FLAGS = -pg

SRC_INC_FLAGS = $(foreach inc_dir, $(DIRS), $(patsubst %, -I%, $(inc_dir)))
LIB_INC_FLAGS := $(LIB_INC_FLAGS) -I$(SRC_DIR)/libs/libarchfpga/include -I$(SRC_DIR)/libs/libcommon_c++/SRC/TIO_InputOutputHandlers -I$(SRC_DIR)/libs/pugixml/src
TEST_INC_FLAGS = -I$(SRC_DIR)/tests/UnitTest++/src

LIB_PATH := $(LIB_PATH)
LIB := $(LIB) -lboost_program_options -lboost_timer -lboost_system

#SciNet provides boost via these environment variables
ifneq ($(SCINET_BOOST_INC),)
	# Include boost as a system header, since boost::polygon generates -Wshadow
	# warnings if compiled as a non-system header
	LIB_INC_FLAGS := $(LIB_INC_FLAGS) -isystem$(SCINET_BOOST_INC)
endif
ifneq ($(SCINET_BOOST_LIB),)
	LIB_PATH := $(LIB_PATH) -L$(SCINET_BOOST_LIB)
endif


LIB_UNITTESTpp_a = $(SRC_DIR)/tests/UnitTest++/libUnitTest++.a
LINK_STATIC_LIBS = #Libraries to link the main executable to

DEFINES := -D EZXML_NOMMAP -D_POSIX_C_SOURCE -DBUILD_VERSION=\""$(BUILD_VERSION)"\" -DBUILD_DATE=\""$(BUILD_DATE)"\"

INC_FLAGS := $(INC_FLAGS) $(SRC_INC_FLAGS) $(LIB_INC_FLAGS) $(TEST_INC_FLAGS)
CFLAGS = $(INC_FLAGS) $(WARN_FLAGS) $(DEFINES) -std=c++0x
LFLAGS = $(INC_FLAGS) $(LIB_PATH)

#
# Configuration
#
#Build Type
ifneq (,$(findstring release, $(BUILD_TYPE)))
  CFLAGS := $(CFLAGS) $(OPT_FLAGS)
  LFLAGS := $(LFLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  CFLAGS := $(CFLAGS) $(DEBUG_FLAGS)
  LFLAGS := $(LFLAGS) $(DEBUG_FLAGS)
endif
#Profiling?
ifneq (,$(findstring yes, $(PROFILE)))
  CFLAGS := $(CFLAGS) $(PROFILE_FLAGS)
  LFLAGS := $(LFLAGS) $(PROFILE_FLAGS)
endif

################################################################################
# Generated Files    														   #
################################################################################
#Generates object names (e.g. $(SRC_DIR)/main.cpp -> $(OBJ_DIR)/main.o) from the files listed in exe_src_cpp and exe_src_c
main_object := $(patsubst %.cpp, %.o, $(exe_main))
main_object := $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(main_object))

objects_cpp := $(patsubst %.cpp, %.o, $(exe_src_cpp))  #Convert from .cpp to .o
objects_cpp := $(foreach obj, $(objects_cpp), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj))) #Change destination to be in obj/ directory

objects_c := $(patsubst %.c, %.o, $(exe_src_c))
objects_c := $(foreach obj, $(objects_c), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj)))

#The associated directories containing the obj files. sort uniqifies dir list
object_dirs := $(sort $(dir $(main_object)) $(dir $(objects_cpp)) $(dir $(objects_c)))

#Dependencies
dependencies = $(subst .o,.d,$(objects_cpp)) $(subst .o,.d,$(objects_c))

# Test Program
test_objects := $(patsubst %.cpp, %.o, $(test_src_cpp))
test_objects := $(foreach obj, $(test_objects), $(patsubst $(SRC_DIR)/%.o, $(OBJ_DIR)/%.o, $(obj)))
test_object_dirs = $(sort $(dir $(test_objects)))
test_dependencies = $(subst .o,.d,$(test_objects))


################################################################################
# Build Dependancies														   #
################################################################################
.PHONY: clean version ctags

all: build_info version ctags $(exe) test
	
#Header Dependancies from the last compile
-include $(dependencies)
-include $(test_dependencies)

####
# Main Executable
####

#Link exe
$(exe): $(object_dirs) $(main_object) $(objects_cpp) $(objects_c) $(LINK_STATIC_LIBS)
	@echo
	@echo "Linking $(exe)..."
	@$(CXX) -MD -MP $(LFLAGS) $(main_object) $(objects_cpp) $(objects_c) $(LINK_STATIC_LIBS) -o $(exe) $(LIB)
	@echo

#Compile & Generate Dependencies
# The directory follows a "|" to use an existence check instead of the usual
# timestamp check.  Every write to the directory updates the timestamp thus
# without this, all but the last file written to a directory would appear
# to be out of date.
#
# -MMD: Generate dependencies only for non-system headers
# -MP: Create empty/phony targets for all the dependencies of the main file
#      This is supposed to avoid errors with removed header files
$(main_object): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp | $(@D)
	@echo "Compiling $<..."
	@$(CXX) -MMD -MP $(CFLAGS) -c $< -o $@

$(objects_cpp): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp | $(@D)
	@echo "Compiling $<..."
	@$(CXX) -MMD -MP $(CFLAGS) -c $< -o $@

$(objects_c): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.c | $(@D)
	@echo "Compiling $<..."
	@$(CXX) -MMD -MP $(CFLAGS) -c $< -o $@

$(object_dirs):
	@ mkdir -p $@

version:
	@echo "Build version: $(BUILD_VERSION)"
	@echo

####
# Test program
####
test: $(test)
	@echo
	@./$(test)

#Link
# Note that the objects_cpp and objects_c specifically DO NOT containn the main.cpp file.
# This allows the test program (whose main is included in test_objects) to be built against
# The main exe source objects without having duplicate definitions of main
$(test): $(object_dirs) $(test_object_dirs) $(objects_cpp) $(objects_c) $(test_objects) $(LINK_STATIC_LIBS) $(LIB_UNITTESTpp_a)
	@echo
	@echo "Linking $(test)..."
	@$(CXX) -MD -MP $(LFLAGS) $(objects_cpp) $(objects_c) $(test_objects) $(LINK_STATIC_LIBS) $(LIB_UNITTESTpp_a) -o $(test) $(LIB)

#Compile
$(test_objects): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp | $(@D)
	@echo "Compiling $<..."
	@$(CXX) -MMD -MP $(CFLAGS) -c $< -o $@

#Test object directories
$(test_object_dirs):
	@ mkdir -p $@

#Build library
$(LIB_UNITTESTpp_a):
	@echo
	@echo "Building libUnitTest++..."
	@+cd $(dir $@) && make $(notdir $@) #+ allows parallel make


####
# Utilities
####

build_info:
	@echo "Build Info:"
	@echo "  Compiler: $(CXX)"
	@echo "  CFLAGS  : $(CFLAGS)"
	@echo "  LFLAGS  : $(LFLAGS)"
	@echo "  OBJ_DIRS: $(object_dirs)"
	@echo

ctags:
	@echo "Generating ctags..."
	@ctags -R *
	@echo


clean:
	cd $(dir $(LIB_ARCH_FPGA_a))  && make clean
	cd $(dir $(LIB_UNITTESTpp_a)) && make clean
	cd $(dir $(LIB_PUGIXML_a)) && make clean
	rm -f $(exe) $(objects_cpp) $(objects_c) $(dependencies) #Main program
	rm -f $(test) $(test_objects) $(test_dependencies) #Test program
	rm -rf $(object_dirs) $(test_object_dirs) #Generated directories

################################################################################
