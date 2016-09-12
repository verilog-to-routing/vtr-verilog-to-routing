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

.PHONY: clean libpugixml libvtrutil

# Standalone executable to test architecture reader
$(EXE): main.o libarchfpga.a libpugixml libvtrutil
	$(CC) main.o -o $(EXE) $(INC_FLAGS) -L. -L../libpugixml -L../libvtrutil -L ../liblog -lm -larchfpga -lpugixml -lvtrutil -llog 
 

libarchfpga.a: $(OBJS)
	ar rcs $@ $(OBJS)

libpugixml:
	@ cd ../libpugixml && make libpugixml.a

libvtrutil:
	@ cd ../libvtrutil && make libvtrutil.a

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)

clean :
	@ rm -f libarchfpga.a
	@ rm -f $(OBJS) $(OBJS:.o=.d)
	@ rm -f read_arch
	@ rm -f main.o main.d
	@ cd ../libpugixml && make clean
	@ cd ../libvtrutil && make clean
