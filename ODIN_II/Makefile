
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

CMAKE_ARGS = -DVTR_IPO_BUILD="off"

BUILD_DIR=../build

.PHONY: help build debug test large_test

help:
	@echo -e "\n\
	The Following options are available\n\n\
		build			build using the VTR_ROOT makefile \n\
		debug			build using the VTR_ROOT makefile with debug flags and extra warning flags for ODIN only\n\
		clean			remove the build file for ODIN only\n\
		test			run the complete battery of test before commiting changes or to assert functionality\n\
		large_test		run the complete battery of test before merging changes\n\
	"

init:
	mkdir -p $(BUILD_DIR)

build: clean init
	cd $(BUILD_DIR);\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) .. &&\
	$(BUILDER)

debug: clean init
	cd $(BUILD_DIR);\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DODIN_DEBUG=on .. &&\
	$(BUILDER)

clean:
	$(RM) -Rf $(BUILD_DIR)/CMakeCache.txt
	$(RM) -Rf $(BUILD_DIR)/ODIN_II

test:
	./verify_odin.sh --test pre_commit --nb_of_process $(NB_OF_PROCESS) --limit_ressource

large_test:
	./verify_odin.sh --test pre_merge --nb_of_process $(NB_OF_PROCESS) --limit_ressource