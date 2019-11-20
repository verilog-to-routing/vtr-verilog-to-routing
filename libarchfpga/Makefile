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
INC_FLAGS = -Iinclude -I../printhandler/SRC/TIO_InputOutputHandlers
LIB_FLAGS = rcs

EXE = read_arch

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) -MD -MP

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

SRC = read_xml_arch_file.c read_xml_util.c ezxml.c ReadLine.c util.c
OBJS = $(SRC:.c=.o)

DEPS = $(OBJS:.o=.d) main.d

# Standalone executable to test architecture reader
$(EXE): main.o libarchfpga.a
	$(CC) main.o -o $(EXE) $(INC_FLAGS) -L. -lm -larchfpga
 

libarchfpga.a: $(OBJS) ../pcre/libpcre.a ../printhandler/libprinthandler.a
	 cp ../printhandler/libprinthandler.a $@
	 ar rcs $@ $(OBJS)

../pcre/libpcre.a:
	@ cd ../pcre && make

../printhandler/libprinthandler.a:
	@ cd ../printhandler && make

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)



clean :
	@ rm -f libarchfpga.a
	@ rm -f $(OBJS) $(OBJS:.o=.d)
	@ rm -f read_arch
	@ rm -f main.o main.d

