/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

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

#include "odin_types.h"

typedef struct soft_sub_structure_t
{
  char *type;
	char *name;
	int bitsize;
}soft_sub_structure;

void read_soft_def_file(t_model *hard_adder_models);
soft_sub_structure *fetch_blk(std::string, int width);


#endif
