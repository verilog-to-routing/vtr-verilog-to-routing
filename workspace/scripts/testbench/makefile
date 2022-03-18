# Makefile for Writing Make Files Example
 
# *****************************************************
# Variables to control Makefile operation
 
CC = g++
CFLAGS = -Wall -g
 
# ****************************************************
# Targets needed to bring the executable up to date
 
tb_build_switchblocks: tb_build_switchblocks.o
	$(CC) $(CFLAGS) -o tb_build_switchblocks tb_build_switchblocks.o
 
# The main.o target can be written more simply
 
tb_build_switchblocks.o: src/tb_build_switchblocks.cpp
	$(CC) $(CFLAGS) -c src/tb_build_switchblocks.cpp