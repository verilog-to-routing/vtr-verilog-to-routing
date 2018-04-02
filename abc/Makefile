
CC   := gcc
CXX  := g++
LD   := $(CXX)

MSG_PREFIX ?=
ABCSRC = .

$(info $(MSG_PREFIX)Using CC=$(CC))
$(info $(MSG_PREFIX)Using CXX=$(CXX))
$(info $(MSG_PREFIX)Using LD=$(LD))

PROG := abc
OS := $(shell uname -s)

MODULES := \
	$(wildcard src/ext*) \
	src/base/abc src/base/abci src/base/cmd src/base/io src/base/main src/base/exor \
	src/base/ver src/base/wlc src/base/acb src/base/bac src/base/cba src/base/pla src/base/test \
	src/map/mapper src/map/mio src/map/super src/map/if \
	src/map/amap src/map/cov src/map/scl src/map/mpm \
	src/misc/extra src/misc/mvc src/misc/st src/misc/util src/misc/nm \
	src/misc/vec src/misc/hash src/misc/tim src/misc/bzlib src/misc/zlib \
	src/misc/mem src/misc/bar src/misc/bbl src/misc/parse \
	src/opt/cut src/opt/fxu src/opt/fxch src/opt/rwr src/opt/mfs src/opt/sim \
	src/opt/ret src/opt/fret src/opt/res src/opt/lpk src/opt/nwk src/opt/rwt \
	src/opt/cgt src/opt/csw src/opt/dar src/opt/dau src/opt/dsc src/opt/sfm src/opt/sbd \
	src/sat/bsat src/sat/xsat src/sat/satoko src/sat/csat src/sat/msat src/sat/psat src/sat/cnf src/sat/bmc src/sat/glucose \
	src/bool/bdc src/bool/deco src/bool/dec src/bool/kit src/bool/lucky \
	src/bool/rsb src/bool/rpo \
	src/proof/pdr src/proof/abs src/proof/live src/proof/ssc src/proof/int \
	src/proof/cec src/proof/acec src/proof/dch src/proof/fraig src/proof/fra src/proof/ssw \
	src/aig/aig src/aig/saig src/aig/gia src/aig/ioa src/aig/ivy src/aig/hop \
	src/aig/miniaig

all: $(PROG)
default: $(PROG)

ARCHFLAGS_EXE ?= ./arch_flags

$(ARCHFLAGS_EXE) : arch_flags.c
	$(CC) arch_flags.c -o $(ARCHFLAGS_EXE)

INCLUDES += -I$(ABCSRC)/src

# Use C99 stdint.h header for platform-dependent types
ifdef ABC_USE_STDINT_H
    ARCHFLAGS ?= -DABC_USE_STDINT_H=1
else
    ARCHFLAGS ?= $(shell $(CC) $(ABCSRC)/arch_flags.c -o $(ARCHFLAGS_EXE) && $(ARCHFLAGS_EXE))
endif

ARCHFLAGS := $(ARCHFLAGS)

OPTFLAGS  ?= -g -O

CFLAGS    += -Wall -Wno-unused-function -Wno-write-strings -Wno-sign-compare $(ARCHFLAGS)
ifneq ($(findstring arm,$(shell uname -m)),)
	CFLAGS += -DABC_MEMALIGN=4
endif

# compile ABC using the C++ comipler and put everything in the namespace $(ABC_NAMESPACE)
ifdef ABC_USE_NAMESPACE
  CFLAGS += -DABC_NAMESPACE=$(ABC_USE_NAMESPACE) -fpermissive
  CC := $(CXX)
  $(info $(MSG_PREFIX)Compiling in namespace $(ABC_NAMESPACE))
endif

# compile CUDD with ABC
ifndef ABC_USE_NO_CUDD
  CFLAGS += -DABC_USE_CUDD=1
  MODULES += src/bdd/cudd src/bdd/extrab src/bdd/dsd src/bdd/epd src/bdd/mtr src/bdd/reo src/bdd/cas src/bdd/bbr src/bdd/llb
  $(info $(MSG_PREFIX)Compiling with CUDD)
endif

ABC_READLINE_INCLUDES ?=
ABC_READLINE_LIBRARIES ?= -lreadline

# whether to use libreadline
ifndef ABC_USE_NO_READLINE
  CFLAGS += -DABC_USE_READLINE $(ABC_READLINE_INCLUDES)
  LIBS += $(ABC_READLINE_LIBRARIES)
  ifeq ($(OS), FreeBSD)
    CFLAGS += -I/usr/local/include
    LDFLAGS += -L/usr/local/lib
  endif
  $(info $(MSG_PREFIX)Using libreadline)
endif

# whether to compile with thread support
ifndef ABC_USE_NO_PTHREADS
  CFLAGS += -DABC_USE_PTHREADS
  LIBS += -lpthread
  $(info $(MSG_PREFIX)Using pthreads)
endif

# whether to compile into position independent code
ifdef ABC_USE_PIC
  CFLAGS += -fPIC
  LIBS += -fPIC
  $(info $(MSG_PREFIX)Compiling position independent code)
endif

# whether to echo commands while building
ifdef ABC_MAKE_VERBOSE
  VERBOSE=
else
  VERBOSE=@
endif

# Set -Wno-unused-bug-set-variable for GCC 4.6.0 and greater only
ifneq ($(or $(findstring gcc,$(CC)),$(findstring g++,$(CC))),)
empty:=
space:=$(empty) $(empty)

GCC_VERSION=$(shell $(CC) -dumpversion)
GCC_MAJOR=$(word 1,$(subst .,$(space),$(GCC_VERSION)))
GCC_MINOR=$(word 2,$(subst .,$(space),$(GCC_VERSION)))

$(info $(MSG_PREFIX)Found GCC_VERSION $(GCC_VERSION))
ifeq ($(findstring $(GCC_MAJOR),0 1 2 3),)
$(info $(MSG_PREFIX)Found GCC_MAJOR>=4)
ifeq ($(findstring $(GCC_MINOR),0 1 2 3 4 5),)
$(info $(MSG_PREFIX)Found GCC_MINOR>=6)
CFLAGS += -Wno-unused-but-set-variable
endif
endif

endif

# LIBS := -ldl -lrt
LIBS += -lm
ifneq ($(OS), FreeBSD)
  LIBS += -ldl
endif

ifneq ($(findstring Darwin, $(shell uname)), Darwin)
   LIBS += -lrt
endif

ifdef ABC_USE_LIBSTDCXX
   LIBS += -lstdc++
   $(info $(MSG_PREFIX)Using explicit -lstdc++)
endif

$(info $(MSG_PREFIX)Using CFLAGS=$(CFLAGS))
CXXFLAGS += $(CFLAGS)

SRC  :=
GARBAGE := core core.* *.stackdump ./tags $(PROG) arch_flags

.PHONY: all default tags clean docs cmake_info

include $(patsubst %, $(ABCSRC)/%/module.make, $(MODULES))

OBJ := \
	$(patsubst %.cc, %.o, $(filter %.cc, $(SRC))) \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(SRC))) \
	$(patsubst %.c, %.o,  $(filter %.c, $(SRC)))  \
	$(patsubst %.y, %.o,  $(filter %.y, $(SRC)))

LIBOBJ := $(filter-out src/base/main/main.o,$(OBJ))

DEP := $(OBJ:.o=.d)

# implicit rules

%.o: %.c
	@echo "$(MSG_PREFIX)\`\` Compiling:" $(LOCAL_PATH)/$<
	$(VERBOSE)$(CC) -c $(OPTFLAGS) $(INCLUDES) $(CFLAGS) $< -o $@

%.o: %.cc
	@echo "$(MSG_PREFIX)\`\` Compiling:" $(LOCAL_PATH)/$<
	$(VERBOSE)$(CXX) -c $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $< -o $@

%.o: %.cpp
	@echo "$(MSG_PREFIX)\`\` Compiling:" $(LOCAL_PATH)/$<
	$(VERBOSE)$(CXX) -c $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $< -o $@

%.d: %.c
	@echo "$(MSG_PREFIX)\`\` Generating dependency:" $(LOCAL_PATH)/$<
	$(VERBOSE)$(ABCSRC)/depends.sh $(CC) `dirname $*.c` $(OPTFLAGS) $(INCLUDES) $(CFLAGS) $< > $@

%.d: %.cc
	@echo "$(MSG_PREFIX)\`\` Generating dependency:" $(LOCAL_PATH)/$<
	$(VERBOSE)$(ABCSRC)/depends.sh $(CXX) `dirname $*.cc` $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $< > $@

%.d: %.cpp
	@echo "$(MSG_PREFIX)\`\` Generating dependency:" $(LOCAL_PATH)/$<
	$(VERBOSE)$(ABCSRC)/depends.sh $(CXX) `dirname $*.cpp` $(OPTFLAGS) $(INCLUDES) $(CXXFLAGS) $< > $@

ifndef ABC_MAKE_NO_DEPS
-include $(DEP)
endif

# Actual targets

depend: $(DEP)

clean:
	@echo "$(MSG_PREFIX)\`\` Cleaning up..."
	$(VERBOSE)rm -rvf $(PROG) lib$(PROG).a $(OBJ) $(GARBAGE) $(OBJ:.o=.d)

tags:
	etags `find . -type f -regex '.*\.\(c\|h\)'`

$(PROG): $(OBJ)
	@echo "$(MSG_PREFIX)\`\` Building binary:" $(notdir $@)
	$(VERBOSE)$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

lib$(PROG).a: $(LIBOBJ)
	@echo "$(MSG_PREFIX)\`\` Linking:" $(notdir $@)
	$(VERBOSE)ar rv $@ $?
	$(VERBOSE)ranlib $@

lib$(PROG).so: $(LIBOBJ)
	@echo "$(MSG_PREFIX)\`\` Linking:" $(notdir $@)
	$(VERBOSE)$(CXX) -shared -o $@ $^ $(LIBS)

docs:
	@echo "$(MSG_PREFIX)\`\` Building documentation." $(notdir $@)
	$(VERBOSE)doxygen doxygen.conf

cmake_info:
	@echo SEPARATOR_CFLAGS $(CFLAGS) SEPARATOR_CFLAGS
	@echo SEPARATOR_CXXFLAGS $(CXXFLAGS) SEPARATOR_CXXFLAGS
	@echo SEPARATOR_LIBS $(LIBS) SEPARATOR_LIBS
	@echo SEPARATOR_SRC $(SRC) SEPARATOR_SRC
