#pragma once
/* Defines the function used to load a vpr constraints file written in XML format into vpr
 * The functions loads up the data structures related to placement and routing constraints
 * according to the data provided in the XML file*/

void load_vpr_constraints_file(const char* read_vpr_constraints_name);
