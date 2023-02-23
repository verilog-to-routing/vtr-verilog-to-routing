ODIN=~/Code/vtr-repo/svn/odin_ii/odin_ii.exe

EXE=verilog_preprocessor

# default: run
default: $(EXE)
# default: arith_test.v

run: Md5Core.vv

$(EXE): $(EXE).c++
	g++ -Wall -Wextra -Werror -pedantic -std=c++0x $< -o $@ -ggdb -D_GLIBCXX_DEBUG

test: Md5Core.v
	less $<

%.v: %.vv $(EXE)
	./$(EXE) < $< > $@

odin: run Md5Core.v
	$(ODIN) -c odin_config.xml
