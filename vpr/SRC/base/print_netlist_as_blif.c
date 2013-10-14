/* 
Print VPR netlist as blif so that formal equivalence checking can be done to verify that the input blif netlist == the internal VPR netlist

Author: Jason Luu
Date: Oct 14, 2013
 */

#include <cstdio>
#include <cstring>
#include <assert.h>
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "print_netlist_as_blif.h"
#include "read_xml_arch_file.h"

void print_preplace_netlist(char *filename) {
	printf("Print pre-place netlist %s\n", filename);
}

