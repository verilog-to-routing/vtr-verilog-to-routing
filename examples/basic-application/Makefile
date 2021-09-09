########################################################################
## Makefile for basic_application
########################################################################

# turn on/off verbosity with 'make {all|release|debug|clean} VERBOSE=1'. Default is silent.
ifeq ($(VERBOSE),1)
	ECHO := 
else
	ECHO := @
endif

# the compiler
CXX = g++

# target EXE
TARGET_DIR = .
TARGET = basic_application

# Resource files
RES_TARGET = resources.C
RES_XML = .gresource.xml

# the version of GTK being used
GTK_VERSION_NUM = 3.0

# the base directory of EZGL
EZGL_DIR = ../..

# get the source and header files for the application and from EZGL
SRCS = $(wildcard ./*.cpp ./$(RES_TARGET) $(EZGL_DIR)/src/*.cpp)
HDRS = $(wildcard ./*.h $(EZGL_DIR)/include/ezgl/*.hpp)

# the GTK include directories. Runs "pkg-config --cflags gtk+-3.0" to get the include directories.
GTK_INCLUDE_DIRS := $(shell pkg-config --cflags gtk+-$(GTK_VERSION_NUM) x11)

# the GTK libraries. Runs "pkg-config --libs gtk+-3.0" to get the libraries to include for GTK.
GTK_LIBS := $(shell pkg-config --libs gtk+-$(GTK_VERSION_NUM) x11)

# set the include directories
INC_DIRS = $(EZGL_DIR)/include $(EZGL_DIR)/include/ezgl

# compiler flags
CXX_FLAGS = -g -Wall -std=c++14

# GLIB resource compiler
GLIB_COMPILE_RESOURCES = $(shell pkg-config --variable=glib_compile_resources gio-2.0)

# resource files
resources = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies $(RES_XML))

# 'make all' target
all: $(RES_TARGET) $(TARGET_DIR)/$(TARGET)

# create the exe
$(TARGET_DIR)/$(TARGET) : Makefile $(SRCS)
	$(ECHO) $(CXX) $(CXX_FLAGS) $(foreach D,$(INC_DIRS),-I$D) $(GTK_INCLUDE_DIRS) $(SRCS) $(GTK_LIBS) -o $(TARGET_DIR)/$(TARGET)

# create the resource file
$(RES_TARGET): $(RES_XML) $(resources)
	$(ECHO) $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-source $(RES_XML) --target=$(RES_TARGET) 

# clean the EXE 
clean:
	$(ECHO) rm -f $(TARGET_DIR)/$(TARGET)
	$(ECHO) rm -f $(RES_TARGET)

# 'make release' is the same as 'make all' but the -O3 flag is added
release: all
CXX_FLAGS += -O3

# 'make debug' is the same as 'make all'. Currently nothing is added here
debug: all


.PHONY: all release debug clean
