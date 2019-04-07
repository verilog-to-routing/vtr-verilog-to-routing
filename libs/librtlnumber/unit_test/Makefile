# If the first argument is "run"...
ifeq (build,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
ifeq (run,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
ifeq (gdb,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
ifeq (valgrind,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
ifeq (debug,$(firstword $(MAKECMDGOALS)))	
  # use the rest as arguments for "make"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif

INCLUDE =-I../src/include

BIN = bin/exec.out

C = clang++ -std=c++14 -lpthread

cleanup_flags=\
-ferror-limit=1000 \
-Werror \
-Wpedantic \
-Weverything \
-Wall \
-Wno-c++98-compat \
-Wno-unused-parameter \
-g -O0 -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls

PHONY: error

error: 
	echo "can only use 'clean', 'debug <testname>.cpp', 'build <testname>.cpp' or 'run <arguments>'"

debug:
	mkdir -p bin
	$(C) -ggdb $(cleanup_flags) $(INCLUDE) $(RUN_ARGS) -o $(BIN)

build:
	mkdir -p bin
	$(C) $(INCLUDE) $(RUN_ARGS) -o $(BIN)

run:
	$(BIN) $(RUN_ARGS) 

valgrind: build
	valgrind --tool=helgrind $(BIN) $(RUN_ARGS) 

gdb:
	gdb --args $(BIN) $(RUN_ARGS)

clean:
	$(RM) -Rf bin

