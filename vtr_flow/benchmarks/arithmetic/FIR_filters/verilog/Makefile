ODIN=odin_II.exe

generate_fir: generate_fir.c++
	g++ -Wall -Wextra -Werror -pedantic -std=c++0x generate_fir.c++ -o generate_fir

test: generate_fir
	./generate_fir | less

run: generate_fir
	./generate_fir > fir.v

odin: run
	$(ODIN) -c odin_config.xml