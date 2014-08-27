ODIN=~/Code/vtr-repo/svn/ODIN_II/odin_II.exe

EXE=verilog_preprocessor

# default: run
default: $(EXE)
# default: arith_test.vv

run: Md5Core.vv

$(EXE): $(EXE).c++
	g++ -Wall -Wextra -Werror -pedantic -std=c++11 $< -o $@ -ggdb -D_GLIBCXX_DEBUG

test: Md5Core.vv
	less $<

%.vv: %.v $(EXE)
	./$(EXE) < $< > $@

odin: run Md5Core.vv
	$(ODIN) -c odin_config.xml
