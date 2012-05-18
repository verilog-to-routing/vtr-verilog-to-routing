################################   MAKEFILE OPTIONS     ####################################

# By default, libvpr's build type (debug/release) is inherited from VPR's makefile.  
# However, by uncommenting out the line BUILD_TYPE = debug, you can override this 
# and set libvpr's build type independently.

# BUILD_TYPE = debug

# (can be debug or release)

#############################################################################################

CC = gcc
AR = ar

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -D__USE_FIXED_PROTOTYPES__ -pedantic -Wmissing-prototypes -Wshadow -Wcast-align 
DEBUG_FLAGS = -g
OPT_FLAGS = -O3
INC_FLAGS = -Iinclude
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

libvpr_6.a: $(OBJS)
	$(AR) $(LIB_FLAGS) $@ $(OBJS)
	cp libvpr_6.a libvpr.a

# Standalone executable to test architecture reader
$(EXE): main.o libvpr_6.a
	$(CC) main.o -o $(EXE) $(INC_FLAGS) -L. -lm -lvpr_6
	cp libvpr_6.a libvpr.a
 
 
%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)



clean :
	rm -f libvpr_6.a
	rm -f libvpr.a
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f read_arch

