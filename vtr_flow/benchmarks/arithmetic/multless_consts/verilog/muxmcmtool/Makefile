all:	kmult

kmult:	kmult.cpp table.cpp table.h chains.cpp chains.h
	g++ -O2 kmult.cpp table.cpp chains.cpp -o kmult


gdb:	kmult.cpp table.cpp chains.cpp chains.h
	g++ -g -O0 kmult.cpp table.cpp chains.cpp -o kmult

draw:
	dot -Tps module.dot -o module.ps
	gv module.ps&

clean:	
	rm -f kmult
