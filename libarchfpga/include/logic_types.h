/*
 Data types describing the logic (technology-mapped) models that the architecture can implement.
 Logic models include LUT (.names), flipflop (.latch), inpad, outpad, memory slice, etc

 Date: February 19, 2009
 Authors: Jason Luu and Kenneth Kent
 */

#ifndef LOGIC_TYPES_H
#define LOGIC_TYPES_H

#include "util.h"

/* 
 Logic model data types
 A logic model is described by its I/O ports and function name
 */
enum PORTS {
	IN_PORT, OUT_PORT, INOUT_PORT, ERR_PORT
};
typedef struct s_model_ports {
	enum PORTS dir; /* port direction */
	char *name; /* name of this port */
	int size; /* maximum number of pins */
	int min_size; /* minimum number of pins */
	boolean is_clock; /* clock? */
	boolean is_non_clock_global; /* not a clock but is a special, global, control signal (eg global asynchronous reset, etc) */
	struct s_model_ports *next; /* next port */

	int index; /* indexing for array look-up */
} t_model_ports;

typedef struct s_model {
	char *name; /* name of this logic model */
	t_model_ports *inputs; /* linked list of input/clock ports */
	t_model_ports *outputs; /* linked list of output ports */
	void *instances;
	int used;
	struct s_linked_vptr *pb_types; /* Physical block types that implement this model */
	struct s_model *next; /* next model (linked list) */

	int index;
} t_model;

#endif

