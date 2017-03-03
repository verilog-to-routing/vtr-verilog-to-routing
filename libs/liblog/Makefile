################################   MAKEFILE OPTIONS     ####################################

# By default, liblog's build type (debug/release) is inherited from VPR's makefile.  
# However, by uncommenting out the line BUILD_TYPE = debug, you can override this 
# and set liblog's build type independently.

# BUILD_TYPE = release

# (can be debug or release)

#############################################################################################

CC = g++
AR = ar


SRC_DIR = src
OBJ_DIR = obj

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -D__USE_FIXED_PROTOTYPES__ -pedantic -Wshadow -Wcast-align -D_POSIX_SOURCE -Wno-write-strings
DEBUG_FLAGS = -g
OPT_FLAGS = -O3
INC_FLAGS = -I$(SRC_DIR)
LIB_FLAGS = rcs

EXE = test_log

FLAGS = $(INC_FLAGS) $(WARN_FLAGS) -MD -MP

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

SRC = log.c
OBJS = $(SRC:.c=.o)

DEPS = $(OBJS:.o=.d) main.d

# Standalone executable to test logger
$(EXE): $(OBJ_DIR)/main.o liblog.a
	$(CC) $(OBJ_DIR)/main.o -o $(EXE) $(INC_FLAGS) -L. -lm -llog
 

liblog.a: $(OBJ_DIR)/$(OBJS)
	 ar rcs $@ $(OBJ_DIR)/$(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)

clean :
	@ rm -f liblog.a
	@ rm -f $(OBJ_DIR)/$(OBJS) $(OBJ_DIR)/$(OBJS:.o=.d)
	@ rm -f $(EXE)
	@ rm -f $(OBJ_DIR)/main.o $(OBJ_DIR)/main.d

