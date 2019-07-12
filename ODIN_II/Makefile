ODIN_ROOT=$(PWD)
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
ODIN_BUILD_DIR=$(BUILD_DIR)/ODIN_II

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

_init:
	mkdir -p $(ODIN_BUILD_DIR)

clean:
	$(RM) -f $(ODIN_BUILD_DIR)/.*.build
	$(RM) -Rf $(BUILD_DIR)/CMakeCache.txt
	$(RM) -Rf $(BUILD_DIR)/ODIN_II

_build_it:
	cd $(BUILD_DIR) &&\
	$(BUILDER)

$(ODIN_BUILD_DIR)/.%.build: clean _init
	touch $@

_set_build: $(ODIN_BUILD_DIR)/.regular.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) ..

_set_debug: $(ODIN_BUILD_DIR)/.debug.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DODIN_DEBUG=on ..

_set_warn: $(ODIN_BUILD_DIR)/.warn.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DODIN_WARN=on ..

_set_gcov: $(ODIN_BUILD_DIR)/.gcov.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DODIN_COVERAGE=on ..

_set_clang_tidy: $(ODIN_BUILD_DIR)/.tidy.build
	cd $(BUILD_DIR) &&\
	cmake  $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DODIN_TIDY=on ..

build: _set_build _build_it

debug: _set_debug _build_it

warn: _set_warn _build_it

gcov: _set_gcov _build_it

clang_tidy: _set_clang_tidy _build_it

scrub:
	find SRC/ -type f \( -iname \*.gcno -or -iname \*.gcda -or -iname \*.gcov \) -exec rm -f {} \; 
	./verify_odin.sh --clean

gcovr:
	find $(BUILD_DIR)/ODIN_II/CMakeFiles/libodin_ii.dir -type f \( -iname \*.gcno -or -iname \*.gcda -or -iname \*.gcov \) -exec cp {} SRC/ \; 
	gcovr --html -s -o coverage_report.html -r .;
	find SRC/ -type f \( -iname \*.gcno -or -iname \*.gcda -or -iname \*.gcov \) -exec rm -f {} \; 

cppcheck:
	cppcheck . 1> /dev/null 

test:
	./verify_odin.sh --test pre_commit --nb_of_process $(NB_OF_PROCESS) --limit_ressource

large_test:
	./verify_odin.sh --test pre_merge --nb_of_process $(NB_OF_PROCESS) --limit_ressource