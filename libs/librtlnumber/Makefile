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

INCLUDE =-Isrc/include
SRC =src/*.cpp

BIN = rtl_number

C = clang++ -std=c++14 -lpthread

cleanup_flags=\
-ferror-limit=1000 \
-Werror \
-Wpedantic \
-Weverything \
-Wall \
-ggdb -O0 -g \
-Wno-c++98-compat \
-Wno-padded
#  \
# -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls

PHONY: error

error: 
	echo "can only use 'clean', 'debug <testname>.cpp', 'build <testname>.cpp' or 'run <arguments>'"

debug: clean
	mkdir -p bin
	$(C) $(cleanup_flags) $(INCLUDE) $(SRC) main.cpp -o $(BIN)

build: clean
	$(C) $(INCLUDE) $(SRC) main.cpp -o $(BIN)

run:
	./$(BIN) $(RUN_ARGS) 

valgrind: build
	valgrind --tool=helgrind $(BIN) $(RUN_ARGS) 

gdb:
	gdb --args $(BIN) $(RUN_ARGS)

clean:
	$(RM) -Rf bin

