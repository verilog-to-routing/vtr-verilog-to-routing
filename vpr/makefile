# This makefile is written for gcc running under Solaris on a SPARCstation.
# To compile VPR on other systems, you may have to change:
# (1) CC to the name of your C compiler.
# (2) LIB_DIR to point at your directory of X11 libraries (libX11.a, etc.)
# (3) On many systems, the -R/usr/openwin/lib option on the LIB line
#     will have to be removed.
# (4) X11_INCLUDE to point at the directory containing "x11/xlib.h" etc.
# (5) OPT_FLAGS should be changed to whatever options turn on maximum 
#     optimization in your compiler.
# (6) If your system does not support X11 (e.g. you're running on Windows NT)
#     then add -DNO_GRAPHICS to the end of the "FLAGS =" line.

CC = gcc 

#SunOS lines below.
#LIB_DIR = -L/usr/lib/X11R5
#LIB = -static -lX11 -lm
#X11_INCLUDE = -I/usr/include

# On many non-Solaris machines, use LIB_DIR = -L/usr/lib/X11R5

LIB_DIR = -L/usr/openwin/lib


# Sometimes get errors under Solaris if you don't use the -R option
# to (I think) put info about where the shared object libraries are
# right into the binary.  Shouldn't be necessary, but it is on our machines.

LIB = -lX11 -lm -R/usr/openwin/lib

X11_INCLUDE = -I/usr/openwin/include

# Overly strict flags line below.  Lots of useless warnings, but can 
# let you look for redudant declarations. 
# To avoid redundant declarations here I use -D__STDC instead of
# -D__USE_FIXED_PROTOTYPES, but that means some prototypes are missing.

#FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__STDC__ -ansi -pedantic -Wredundant-decls -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE

#Flags to be passed to the compiler.  First is for strict warnings, 
#second for interactive debugging and third for optimization. 

#-D_POSIX_SOURCE stops extra declarations from being included in math.h
#and causing -Wshadow to complain about conflicts with y1 in math.h
#(Bessel function 1 of the second kind) 

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE
DEBUG_FLAGS = -g
OPT_FLAGS = -O3

#FLAGS = $(WARN_FLAGS)
#FLAGS = $(DEBUG_FLAGS) 
FLAGS = $(OPT_FLAGS)

#Uncomment line below if X Windows isn't installed on your system.
#FLAGS = $(OPT_FLAGS) -DNO_GRAPHICS

#Useful flags on HP machines
#DEBUG_FLAGS = -Aa -g
#OPT_FLAGS = -Aa +O3

EXE = vpr

OBJ = main.o util.o read_netlist.o print_netlist.o check_netlist.o read_arch.o place_and_route.o place.o route_common.o route_timing.o route_tree_timing.o route_breadth_first.o draw.o graphics.o stats.o segment_stats.o rr_graph.o rr_graph2.o rr_graph_sbox.o rr_graph_util.o rr_graph_timing_params.o rr_graph_indexed_data.o rr_graph_area.o check_rr_graph.o check_route.o hash.o heapsort.o read_place.o net_delay.o path_delay.o path_delay2.o vpr_utils.o timing_place_lookup.o timing_place.o

SRC = main.c util.c read_netlist.c print_netlist.c check_netlist.c read_arch.c place_and_route.c place.c route_common.c route_timing.c route_tree_timing.c route_breadth_first.c draw.c graphics.c stats.c segment_stats.c rr_graph.c rr_graph2.c rr_graph_sbox.c rr_graph_util.c rr_graph_timing_params.c rr_graph_indexed_data.c rr_graph_area.c check_rr_graph.c check_route.c hash.c heapsort.c read_place.c net_delay.c path_delay.c path_delay2.c test_h.c vpr_utils.c timing_place_lookup.c timing_place.c

H = util.h vpr_types.h globals.h graphics.h read_netlist.h print_netlist.h check_netlist.h read_arch.h stats.h segment_stats.h draw.h place_and_route.h place.h route_export.h route_common.h route_timing.h route_tree_timing.h route_breadth_first.h rr_graph.h rr_graph2.h rr_graph_sbox.h rr_graph_util.h rr_graph_timing_params.h rr_graph_indexed_data.h rr_graph_area.h check_rr_graph.h check_route.h hash.h heapsort.h read_place.h path_delay.h path_delay2.h net_delay.h vpr_utils.h timing_place_lookup.h timing_place.h


# I haven't been able to make -static work under Solaris.  Use shared
# libraries all the time.

# Add purify before $(CC) in the link line below to run purify on VPR.

$(EXE): $(OBJ) test_h.o
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)

# test_h.c is a dummy file -- it just checks the header files against each
# other to see if any global variable definitions conflict in the various
# header files.

test_h.o: test_h.c $(H)
	$(CC) -c $(WARN_FLAGS) -Wredundant-decls test_h.c


main.o: main.c $(H)
	$(CC) -c $(FLAGS) main.c

read_netlist.o: read_netlist.c $(H)
	$(CC) -c $(FLAGS) read_netlist.c

print_netlist.o: print_netlist.c $(H)
	$(CC) -c $(FLAGS) print_netlist.c

check_netlist.o: check_netlist.c $(H)
	$(CC) -c $(FLAGS) check_netlist.c

read_arch.o: read_arch.c $(H)
	$(CC) -c $(FLAGS) read_arch.c

place_and_route.o: place_and_route.c $(H)
	$(CC) -c $(FLAGS) place_and_route.c

place.o: place.c $(H)
	$(CC) -c $(FLAGS) place.c

route_common.o: route_common.c $(H)
	$(CC) -c $(FLAGS) route_common.c

route_timing.o: route_timing.c $(H)
	$(CC) -c $(FLAGS) route_timing.c

route_tree_timing.o: route_tree_timing.c $(H)
	$(CC) -c $(FLAGS) route_tree_timing.c

route_breadth_first.o: route_breadth_first.c $(H)
	$(CC) -c $(FLAGS) route_breadth_first.c

stats.o: stats.c $(H)
	$(CC) -c $(FLAGS) stats.c

segment_stats.o: segment_stats.c $(H)
	$(CC) -c $(FLAGS) segment_stats.c

util.o: util.c $(H)
	$(CC) -c $(FLAGS) util.c

draw.o: draw.c $(H)
	$(CC) -c $(FLAGS) draw.c

rr_graph.o: rr_graph.c $(H)
	$(CC) -c $(FLAGS) rr_graph.c

rr_graph2.o: rr_graph2.c $(H)
	$(CC) -c $(FLAGS) rr_graph2.c

rr_graph_sbox.o: rr_graph_sbox.c $(H)
	$(CC) -c $(FLAGS) rr_graph_sbox.c

rr_graph_util.o: rr_graph_util.c $(H)
	$(CC) -c $(FLAGS) rr_graph_util.c

rr_graph_timing_params.o: rr_graph_timing_params.c $(H)
	$(CC) -c $(FLAGS) rr_graph_timing_params.c

rr_graph_indexed_data.o: rr_graph_indexed_data.c $(H)
	$(CC) -c $(FLAGS) rr_graph_indexed_data.c

rr_graph_area.o: rr_graph_area.c $(H)
	$(CC) -c $(FLAGS) rr_graph_area.c

check_rr_graph.o: check_rr_graph.c $(H)
	$(CC) -c $(FLAGS) check_rr_graph.c

graphics.o: graphics.c $(H)
	$(CC) -c $(FLAGS) $(X11_INCLUDE) graphics.c

check_route.o: check_route.c $(H)
	$(CC) -c $(FLAGS) check_route.c

hash.o: hash.c $(H)
	$(CC) -c $(FLAGS) hash.c

heapsort.o: heapsort.c $(H)
	$(CC) -c $(FLAGS) heapsort.c

read_place.o: read_place.c $(H)
	$(CC) -c $(FLAGS) read_place.c

net_delay.o: net_delay.c $(H)
	$(CC) -c $(FLAGS) net_delay.c

path_delay.o: path_delay.c $(H)
	$(CC) -c $(FLAGS) path_delay.c

path_delay2.o: path_delay2.c $(H)
	$(CC) -c $(FLAGS) path_delay2.c

vpr_utils.o: vpr_utils.c $(H)
	$(CC) -c $(FLAGS) vpr_utils.c

timing_place_lookup.o: timing_place_lookup.c $(H)
	$(CC) -c $(FLAGS) timing_place_lookup.c

timing_place.o: timing_place.c $(H)
	$(CC) -c $(FLAGS) timing_place.c
