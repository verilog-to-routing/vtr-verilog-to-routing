/*********************************************************************************************
*	lut_recog.h
* Header file allowing the vqm2blif convertor to recognize certain VQM primitives as LUTs
* so it can elaborate them into the BLIF standard ".names" structure. 
*
* For more, see lut_recog.cpp
*********************************************************************************************/
#ifndef LUTRECOG_H
#define LUTRECOG_H

#include "vqm2blif_util.h"

typedef bool (*CP_function)(t_node*); //Function Pointer Type
typedef map <string, CP_function> lut_support_map;
typedef pair <string, CP_function> lut_support_pair;

extern lut_support_map supported_luts; 
void setup_lut_support_map ();
bool is_lut (t_node* vqm_node);

#endif