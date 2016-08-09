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
LIB_FLAGS = rcs

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) -MD -MP -std=c++0x

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

SRC = src/pugixml.cpp
OBJS = $(SRC:.cpp=.o)

DEPS = $(OBJS:.o=.d)
 

libpugixml.a: $(OBJS)
	 ar rcs $@ $(OBJS)

%.o: %.cpp
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)

clean :
	@ rm -f libpugixml.a
	@ rm -f $(OBJS) $(OBJS:.o=.d)
