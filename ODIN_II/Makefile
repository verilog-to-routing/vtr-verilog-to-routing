ODIN_ROOT=$(PWD)
NB_OF_PROCESS ?= $(shell python -c "import multiprocessing; print(multiprocessing.cpu_count())")

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

_init: clean
	mkdir -p $(ODIN_BUILD_DIR)

clean:
	$(RM) -f $(ODIN_BUILD_DIR)/.*.build
	$(RM) -Rf $(BUILD_DIR)/CMakeCache.txt
	$(RM) -Rf $(BUILD_DIR)/ODIN_II

define _build_it_gen
_build_it_$(1): _set_$(1)
	cd $(BUILD_DIR) &&\
	echo "Building with $(NB_OF_PROCESS) threads" &&\
	$(BUILDER)

$(1): _build_it_$(1)
endef

$(ODIN_BUILD_DIR)/.%.build: _init
	touch $@

_set_build: $(ODIN_BUILD_DIR)/.regular.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) ..

_set_debug: $(ODIN_BUILD_DIR)/.debug.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DVTR_IPO_BUILD=off -DODIN_DEBUG=on ..

_set_warn: $(ODIN_BUILD_DIR)/.warn.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DVTR_IPO_BUILD=off -DODIN_WARN=on ..

_set_gcov: $(ODIN_BUILD_DIR)/.gcov.build
	cd $(BUILD_DIR) &&\
	cmake $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DVTR_IPO_BUILD=off -DODIN_COVERAGE=on ..

_set_clang_tidy: $(ODIN_BUILD_DIR)/.tidy.build
	cd $(BUILD_DIR) &&\
	cmake  $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DVTR_IPO_BUILD=off -DODIN_TIDY=on ..

_set_sanitize: $(ODIN_BUILD_DIR)/.sanitize.build
	cd $(BUILD_DIR) &&\
	cmake  $(CMAKE_GEN_ARGS) $(CMAKE_ARGS) -DVTR_IPO_BUILD=off -DODIN_SANITIZE=on ..

BUILD_IT_TARGETS = build debug warn gcov clang_tidy sanitize
$(foreach  t,$(BUILD_IT_TARGETS), $(eval $(call _build_it_gen,$(t))))

scrub:
	find SRC/ -type f \( -iname \*.gcno -or -iname \*.gcda -or -iname \*.gcov \) -exec rm -f {} \; 
	./verify_odin.sh --clean

gcovr:
	find $(BUILD_DIR)/ODIN_II/CMakeFiles/libodin_ii.dir -type f \( -iname \*.gcno -or -iname \*.gcda -or -iname \*.gcov \) -exec cp {} SRC/ \; 
	gcovr --html -s -o coverage_report.html -r .;
	find SRC/ -type f \( -iname \*.gcno -or -iname \*.gcda -or -iname \*.gcov \) -exec rm -f {} \; 

cppcheck:
	cppcheck . 1> /dev/null 

test_light_coverage:
	$(MAKE) gcov
	./verify_odin.sh -t regression_test/benchmark/suite/light_suite -j $(NB_OF_PROCESS)
	$(MAKE) gcovr

test_full_coverage:
	$(MAKE) test_light_coverage
	./verify_odin.sh -t regression_test/benchmark/suite/heavy_suite -j $(NB_OF_PROCESS)
	$(MAKE) gcovr

test:
# rebuild with sanitize flags on to run the same test as travis
	$(MAKE) sanitize
	./verify_odin.sh -t regression_test/benchmark/suite/light_suite -j $(NB_OF_PROCESS)
	./verify_odin.sh -t vtr_reg_basic -j $(NB_OF_PROCESS)
	./verify_odin.sh -t vtr_reg_strong -j $(NB_OF_PROCESS)

large_test:
# larger test should run with regular flags to speed up the process
	$(MAKE) build
	./verify_odin.sh -t regression_test/benchmark/suite/heavy_suite -j $(NB_OF_PROCESS)

pre_merge:
	$(MAKE) test
	$(MAKE) large_test
	

