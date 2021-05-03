/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "odin_ii.h"
#include "odin_util.h"
#include "odin_types.h"
#include "odin_globals.h"

#include "ast_util.h"
#include "blif_elaborate.hh"
#include "netlist_utils.h"
#include "netlist_check.h"
#include "simulate_blif.h"

#include "vtr_util.h"
#include "vtr_memory.h"

#include "string_cache.h"
#include "node_creation_library.h"

#include "multipliers.h"
#include "hard_blocks.h"
#include "adders.h"
#include "subtractions.h"

#include "VerilogReader.hh"
#include "BLIF.hh"
#include "OdinBLIFReader.hh"
#include "SubcktBLIFReader.hh"


/**
 * @brief Construct the BLIF object
 * required by compiler
 */
BLIF::BLIF() = default;
/**
 * @brief Destruct the BLIF object
 * to avoid memory leakage
 */
BLIF::~BLIF() = default;