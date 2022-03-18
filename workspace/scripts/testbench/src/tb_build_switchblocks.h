#include <cstring>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <experimental/filesystem>

#include <sys/stat.h>

#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_string_view.h"

#include "vpr_error.h"

#include "physical_types.h"
#include "parse_switchblocks.h"
#include "vtr_expr_eval.h"
#include <fstream>
using vtr::FormulaParser;
using vtr::t_formula_data;

void scratchpad_init();
void tb_build_switchblocks();
void tb_compute_wire_connections();
void tb_compute_wireconn_connections();
