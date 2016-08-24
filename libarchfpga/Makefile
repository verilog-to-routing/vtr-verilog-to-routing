################################   MAKEFILE OPTIONS     ####################################

# By default, libarchfpga's build type (debug/release) is inherited from VPR's makefile.  
# However, by uncommenting out the line BUILD_TYPE = debug, you can override this 
# and set libarchfpga's build type independently.

# BUILD_TYPE = release

# (can be debug or release)

#############################################################################################

CC = g++
AR = ar

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

SRC = read_xml_arch_file.c read_xml_util.c ReadLine.c util.c parse_switchblocks.c echo_arch.c arch_util.c
OBJS = $(SRC:.c=.o)

DEPS = $(OBJS:.o=.d) main.d

.PHONY: clean liblog.a libpugixml libvtrutil

# Standalone executable to test architecture reader
$(EXE): main.o libarchfpga.a
	$(CC) main.o -o $(EXE) $(INC_FLAGS) -L. -lm -larchfpga
 

libarchfpga.a: $(OBJS) liblog libpugixml libvtrutil
	 cp ../liblog/liblog.a $@
	 ar rcs $@ $(OBJS)
	@#Extract libpugixml.a into OBJ/libpugixml and then have ar archive results
	@ cp ../libpugixml/libpugixml.a .
	@ mkdir -p  OBJ/libpugixml/
	@ cd OBJ/libpugixml && ar x ../../libpugixml.a 
	@ ar rcs $@ $(OBJ) OBJ/libpugixml/*.o
	@#Extract libvtrutil.a into OBJ/libvtrutil and then have ar archive results
	@ cp ../libvtrutil/libvtrutil.a .
	@ mkdir -p  OBJ/libvtrutil/
	@ cd OBJ/libvtrutil && ar x ../../libvtrutil.a 
	@ ar rcs $@ $(OBJ) OBJ/libvtrutil/*.o

liblog:
	@ cd ../liblog && make

libpugixml:
	@ cd ../libpugixml && make

libvtrutil:
	@ cd ../libvtrutil && make

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)



clean :
	@ rm -f libarchfpga.a
	@ rm -f $(OBJS) $(OBJS:.o=.d)
	@ rm -f read_arch
	@ rm -f main.o main.d
	@ cd ../libpugixml && make clean
	@ rm -rf $(OBJ_DIR)/libpugixml
	@ cd ../libvtrutil && make clean
	@ rm -rf $(OBJ_DIR)/libvtrutil

