#include <cstring>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <experimental/filesystem>

#include <sys/stat.h>

#include "tb_build_switchblocks.h"

#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_string_view.h"

#include "vpr_error.h"

#include "build_switchblocks.h"
#include "physical_types.h"
#include "parse_switchblocks.h"
#include "vtr_expr_eval.h"
#include <fstream>
using vtr::FormulaParser;
using vtr::t_formula_data;

int main()
{
    printf("'main.cpp' is working\n");
    tb_build_switchblocks();
}