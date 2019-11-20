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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "types.h"
#include "globals.h"
#include "errors.h"
#include "netlist_utils.h"
#include "odin_util.h"
#include "netlist_optimizations.h"
#include "multipliers.h"
#include "memories.h"
#include "adders.h"
#include "subtractions.h"

/*------------------------------------------------------------------------
 * (function: netlist_optimizations_top)
 *----------------------------------------------------------------------*/
void netlist_optimizations_top(netlist_t *netlist)
{
	#ifdef VPR6
	/* Perform a splitting of the multipliers for hard block mults */
	iterate_multipliers(netlist);
	clean_multipliers();

	/* Perform a splitting of any hard block memories */
	iterate_memories(netlist);
	free_memory_lists();

	/* Perform a splitting of the adders for hard block add */
	iterate_adders(netlist);
	clean_adders();

	/* Perform a splitting of the adders for hard block sub */
	iterate_adders_for_sub(netlist);
	clean_adders_for_sub();
	#endif
}

