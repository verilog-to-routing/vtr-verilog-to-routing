/*
 * Data types describing the logic (technology-mapped) models that the architecture can implement.
 * Logic models include LUT (.names), flipflop (.latch), inpad, outpad, memory slice, etc.
 * Logic models are from the internal VPR library, or can be user-defined (both defined in .blif)
 *
 * Date: February 19, 2009
 * Authors: Jason Luu and Kenneth Kent
 */

#ifndef LOGIC_TYPES_H
#define LOGIC_TYPES_H

#include "vtr_list.h"
#include <vector>
#include <string>

/*
 * Logic model data types
 * A logic model is described by its I/O ports and function name
 */
enum PORTS {
    IN_PORT,
    OUT_PORT,
    INOUT_PORT,
    ERR_PORT
};

struct t_model_ports {
    enum PORTS dir = ERR_PORT;                         /* port direction */
    char* name = nullptr;                              /* name of this port */
    int size = 0;                                      /* maximum number of pins */
    int min_size = 0;                                  /* minimum number of pins */
    bool is_clock = false;                             /* clock? */
    bool is_non_clock_global = false;                  /* not a clock but is a special, global, control signal (eg global asynchronous reset, etc) */
    std::string clock;                                 /* The clock associated with this pin (if the pin is sequential) */
    std::vector<std::string> combinational_sink_ports; /* The other ports on this model which are combinationally driven by this port */

    t_model_ports* next = nullptr; /* next port */

    int index = -1; /* indexing for array look-up */
};

struct t_model {
    char* name = nullptr;             /* name of this logic model */
    t_model_ports* inputs = nullptr;  /* linked list of input/clock ports */
    t_model_ports* outputs = nullptr; /* linked list of output ports */
    void* instances = nullptr;
    int used = 0;
    vtr::t_linked_vptr* pb_types = nullptr; /* Physical block types that implement this model */
    t_model* next = nullptr;                /* next model (linked list) */

    int index = -1;
};

#endif
