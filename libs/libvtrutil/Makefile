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

FLAGS = $(WARN_FLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) -I ../liblog/src -MD -MP -std=c++0x

#Utility
join-width = $(subst " ",$1,$(strip $2))

#Version info
VTR_VERSION_MAJOR := 8
VTR_VERSION_MINOR := 0
VTR_VERSION_PATCH := 0
VTR_VERSION_PRERELEASE := dev
VTR_VCS_REVISION := $(shell git describe --always --long --dirty)
VTR_VERSION_SHORT := $(VTR_VERSION_MAJOR).$(VTR_VERSION_MINOR).$(VTR_VERSION_PATCH)-$(VTR_VERSION_PRERELEASE)
VTR_VERSION := $(VTR_VERSION_SHORT)+$(VTR_VCS_REVISION)
VTR_BUILD_TIMESTAMP := $(shell date -R)
VTR_COMPILER := $(CC) on $(shell uname -s -p -r -v)

ifneq (,$(findstring release, $(BUILD_TYPE)))
  FLAGS := $(FLAGS) $(OPT_FLAGS)
else                              # DEBUG build
  FLAGS := $(FLAGS) $(DEBUG_FLAGS)
endif

SRC_DIR = src
OBJ_DIR = obj
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.cpp))
VERSION_SRC = $(OBJ_DIR)/vtr_version.cpp
VERSION_OBJ = $(VERSION_SRC:.cpp=.o)
DEPS = $(OBJS:.o=.d) $(VERSION_OBJ:.o=.d)
 
.PHONY: clean liblog $(OBJ_DIR)/vtr_version.cpp

libvtrutil.a: $(OBJS) $(VERSION_OBJ) liblog
	 ar rcs $@ $(OBJS) $(VERSION_OBJ)

liblog:
	cd ../liblog && make liblog.a

$(OBJS): $(OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) $(FLAGS) -c $< -o $@ 

$(OBJ_DIR)/vtr_version.cpp: $(SRC_DIR)/vtr_version.cpp.in
	 @ echo "VTR_VERSION: $(VTR_VERSION)"
	 @ mkdir -p $(dir $@)
	 @ cp $^ $@
	 @ sed -i 's/@VTR_VERSION@/$(VTR_VERSION)/' $@
	 @ sed -i 's/@VTR_VERSION_SHORT@/$(VTR_VERSION_SHORT)/' $@
	 @ sed -i 's/@VTR_VERSION_MAJOR@/$(VTR_VERSION_MAJOR)/' $@
	 @ sed -i 's/@VTR_VERSION_MINOR@/$(VTR_VERSION_MINOR)/' $@
	 @ sed -i 's/@VTR_VERSION_PATCH@/$(VTR_VERSION_PATCH)/' $@
	 @ sed -i 's/@VTR_VERSION_PRERELEASE@/$(VTR_VERSION_PRERELEASE)/' $@
	 @ sed -i 's/@VTR_VCS_REVISION@/$(VTR_VCS_REVISION)/' $@
	 @ sed -i 's/@VTR_COMPILER_INFO@/$(VTR_COMPILER)/' $@
	 @ sed -i 's/@VTR_BUILD_TIMESTAMP@/$(VTR_BUILD_TIMESTAMP)/' $@

$(OBJ_DIR)/vtr_version.o: $(OBJ_DIR)/vtr_version.cpp
	mkdir -p $(dir $@)
	$(CC) $(FLAGS) -I src -c $< -o $@ 
 
-include $(DEPS)

clean:
	@ rm -f libvtrutil.a
	@ rm -rf $(OBJ_DIR) $(OBJS) $(DEPS) $(VERSION_OBJ)
	@ cd ../liblog && make clean
