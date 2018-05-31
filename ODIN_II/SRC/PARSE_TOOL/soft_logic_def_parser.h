/***
 * this defines the structures for the adders if no file but optimization are requested
 * these are computed individually using adder_tree of each specific bit lenght
 *
 * TODO there is no way to modify this structure at the moment or allow odin to overwrite these at runtime
 * one interesting thing to do would be to wrap vtr within odin and parse the result to optimize the given circuit
 * and run through all the possible branches of the adder size since each bits can have different dependency down the line
 * and having Odin optimize these would allow for even faster circuit specif adder.
 *
 * P.S it is extremely slow! running every branch for each adder_tree up to 128 bit took 4 days on 4 cores.
 *
 * P.S.2 we can currently achieve this manualy using the bash script contained in vtr_flow/aritmetic_tasks/soft_adder
 */

#ifndef SOFT_LOGIC_PARSER_H
#define SOFT_LOGIC_PARSER_H

#include "types.h"

void read_soft_def_file(const char *input_file_name);

#endif
