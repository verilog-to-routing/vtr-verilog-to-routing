################################   MAKEFILE OPTIONS     ####################################

# By default, libarchfpga's build type (debug/release) is inherited from VPR's makefile.  
# However, by uncommenting out the line BUILD_TYPE = debug, you can override this 
# and set libarchfpga's build type independently.

# BUILD_TYPE = release

# (can be debug or release)

#############################################################################################

CC = g++
AR = ar

OBJ_DIR = OBJ

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings
DEBUG_FLAGS = -g
OPT_FLAGS = -O3
INC_FLAGS = -Iinclude -I../liblog/src -I../libpugixml/src -I../libvtrutil/src
LIB_FLAGS = rcs


EXE = read_arch

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) -MD -MP -std=c++0x

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

SRC = read_xml_arch_file.c read_xml_util.c ReadLine.c util.c parse_switchblocks.c echo_arch.c arch_util.c arch_error.c
OBJS = $(SRC:.c=.o)

DEPS = $(OBJS:.o=.d) main.d

.PHONY: clean liblog libpugixml libvtrutil

# Standalone executable to test architecture reader
$(EXE): main.o libarchfpga.a
	$(CC) main.o -o $(EXE) $(INC_FLAGS) -L. -lm -larchfpga
 

libarchfpga.a: $(OBJS) liblog libpugixml libvtrutil
	ar rcs $@ $(OBJS) $(OBJ_DIR)/*/*.o

liblog:
	@ cd ../liblog && make
	@#We copy and extract the libraries and then put them all in libarchfpga
	@ mkdir -p  $(OBJ_DIR)/liblog/
	@ cp ../liblog/liblog.a $(OBJ_DIR)/liblog/.
	@ cd $(OBJ_DIR)/liblog && ar x liblog.a 

libpugixml:
	@ cd ../libpugixml && make
	@#We copy and extract the libraries and then put them all in libarchfpga
	@ mkdir -p  $(OBJ_DIR)/libpugixml/
	@ cp ../libpugixml/libpugixml.a $(OBJ_DIR)/libpugixml/.
	@ cd $(OBJ_DIR)/libpugixml && ar x libpugixml.a 

libvtrutil:
	@ cd ../libvtrutil && make
	@#We copy and extract the libraries and then put them all in libarchfpga
	@ mkdir -p  $(OBJ_DIR)/libvtrutil/
	@ cp ../libvtrutil/libvtrutil.a $(OBJ_DIR)/libvtrutil/.
	@ cd $(OBJ_DIR)/libvtrutil && ar x libvtrutil.a 

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)



clean :
	rm -f libarchfpga.a
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f read_arch
	rm -f main.o main.d
	cd ../libpugixml && make clean
	cd ../libvtrutil && make clean
	cd ../liblog && make clean
	rm -rf $(OBJ_DIR)

