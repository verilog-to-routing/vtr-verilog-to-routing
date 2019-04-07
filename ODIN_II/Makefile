
NB_OF_PROCESS ?= $(shell nproc --all)

################
# build with ninja when doable
################
ifneq ($(shell which ninja | grep -v "not found"),)
  BUILDER := ninja
  CMAKE_GEN_ARGS := -GNinja
else
  BUILDER := make -j$(NB_OF_PROCESS)
  CMAKE_GEN_ARGS := -G'Unix Makefiles'
endif

#Default build type
# Possible values:
#    release
#    debug
BUILD_TYPE ?= release
MAKEFLAGS := -s

BUILD_DIR=../build

.PHONY: help build debug

help:
	@echo -e "\n\
	The Following three options are available\n\n\
		build		build using the VTR_ROOT makefile \n\
		debug		build using the VTR_ROOT makefile with debug flags and extra warning flags for ODIN only\n\
		clean		remove the build file for ODIN only\n\
	"

init:
	mkdir -p $(BUILD_DIR)

build: clean init
	cd $(BUILD_DIR);\
	cmake $(CMAKE_GEN_ARGS) .. &&\
	$(BUILDER)

debug: clean init
	cd $(BUILD_DIR);\
	cmake $(CMAKE_GEN_ARGS) -DODIN_DEBUG=on .. &&\
	$(BUILDER)

clean:
	$(RM) -Rf $(BUILD_DIR)/CMakeCache.txt
	$(RM) -Rf $(BUILD_DIR)/ODIN_II
