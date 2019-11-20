CC = gcc
LIB_DIR = -L/usr/X11R6/lib
LIB = -lm -lX11
X11_INCLUDE = -I/usr/X11R6/include
SRC_DIR = SRC

WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE
DEBUG_FLAGS = -g 
OPT_FLAGS = -O3

#FLAGS = $(WARN_FLAGS) -D EZXML_NOMMAP
FLAGS = $(OPT_FLAGS) -D EZXML_NOMMAP
#FLAGS = $(OPT_FLAGS) -DNO_GRAPHICS -D EZXML_NOMMAP
#FLAGS = $(DEBUG_FLAGS) -pedantic -D EZXML_NOMMAP

EXE = vpr

OBJ = OBJ/main.o \
OBJ/util.o \
OBJ/read_netlist.o \
OBJ/print_netlist.o \
OBJ/check_netlist.o \
OBJ/place_and_route.o \
OBJ/route_common.o \
OBJ/route_timing.o \
OBJ/route_tree_timing.o \
OBJ/route_breadth_first.o \
OBJ/stats.o \
OBJ/segment_stats.o \
OBJ/route_directed_search.o \
OBJ/mst.o \
OBJ/rr_graph.o \
OBJ/rr_graph2.o \
OBJ/rr_graph_sbox.o \
OBJ/rr_graph_util.o \
OBJ/rr_graph_timing_params.o \
OBJ/rr_graph_indexed_data.o \
OBJ/rr_graph_area.o \
OBJ/check_rr_graph.o \
OBJ/check_route.o \
OBJ/hash.o \
OBJ/heapsort.o \
OBJ/read_place.o \
OBJ/net_delay.o \
OBJ/path_delay.o \
OBJ/path_delay2.o \
OBJ/vpr_utils.o \
OBJ/ReadLine.o \
OBJ/ReadOptions.o \
OBJ/OptionTokens.o \
OBJ/SetupVPR.o \
OBJ/CheckSetup.o \
OBJ/ShowSetup.o \
OBJ/CheckOptions.o \
OBJ/CheckArch.o \
OBJ/xml_arch.o \
OBJ/ezxml.o \
OBJ/place.o \
OBJ/place_stats.o \
OBJ/timing_place.o \
OBJ/draw.o\
OBJ/graphics.o\
OBJ/timing_place_lookup.o

H = SRC/util.h\
SRC/vpr_types.h\
SRC/globals.h\
SRC/read_netlist.h\
SRC/print_netlist.h\
SRC/check_netlist.h\
SRC/stats.h\
SRC/segment_stats.h\
SRC/draw.h\
SRC/graphics.h\
SRC/place_and_route.h\
SRC/route_export.h\
SRC/route_common.h\
SRC/route_timing.h\
SRC/route_tree_timing.h\
SRC/route_breadth_first.h\
SRC/rr_graph.h\
SRC/rr_graph2.h\
SRC/rr_graph_sbox.h\
SRC/rr_graph_util.h\
SRC/rr_graph_timing_params.h\
SRC/rr_graph_indexed_data.h\
SRC/rr_graph_area.h\
SRC/check_rr_graph.h\
SRC/check_route.h\
SRC/hash.h\
SRC/heapsort.h\
SRC/read_place.h\
SRC/path_delay.h\
SRC/path_delay2.h\
SRC/net_delay.h\
SRC/vpr_utils.h\
SRC/\
SRC/route_directed_search.h\
SRC/mst.h\
SRC/ReadOptions.h\
SRC/OptionTokens.h\
SRC/SetupVPR.h\
SRC/xml_arch.h\
SRC/ezxml.h\
SRC/place.h\
SRC/place_stats.h\
SRC/timing_place.h\
SRC/timing_place_lookup.h

$(EXE): $(OBJ) Makefile
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)

OBJ/%.o:$(SRC_DIR)/%.c
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(EXE) $(OBJ)

ctags:
	cd $(SRC_DIR) && ctags *.[ch]
