/*
Data types describing the logic models that the architecture can implement.

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

enum PORTS {IN_PORT, OUT_PORT, INOUT_PORT, ERR_PORT};
typedef struct s_model_ports
{
	enum PORTS dir;
	char *name;
	int size;
	int min_size;
	boolean is_clock;
	struct s_model_ports *next;

	int index;
} t_model_ports;

typedef struct s_model
{
	char *name;
	t_model_ports *inputs;
	t_model_ports *outputs;
	void *instances;
	int used;
	struct s_linked_vptr *pb_types;
	struct s_model *next;

	int index;
} t_model;

#endif

