/**
General API for VPR 

VPR is a CAD tool used to conduct FPGA architecture exploration.  It takes, as input, a technology-mapped netlist and a description of the FPGA architecture being investigated.  
VPR then generates a packed, placed, and routed FPGA (in .net, .place, and .route files respectively) that implements the input netlist

Software tools interfacing to VPR should generally call just the functions defined here
For advanced/power users, you can call functions defined elsewhere in VPR or modify the data structures directly at your discretion but be aware that doing so can break the correctness of this tool

 General Usage: 
   1. vpr_init
   2. vpr_pack
   3. vpr_init_pre_place_and_route
   4. vpr_place_and_route
   5. vpr_free_all

If you are a new developer, key files to begin understanding this code base are:
 1.  libarchfpga/physical_types.h - Data structures that define the properties of the FPGA architecture
 2.  vpr_types.h - Very major file that defines the core data structures used in VPR.  This includes detailed architecture information, user netlist data structures, and data structures that describe the mapping between those two.
 3.  globals.h - Defines the global variables used by VPR.

Author: Jason Luu
June 21, 2012
*/

#ifndef VPR_API_H
#define VPR_API_H

#include "physical_types.h"
#include "vpr_types.h"
#include "ReadOptions.h"

/* Main VPR Operations */
void vpr_init(INP int argc, INP char **argv, OUTP t_options *options, OUTP t_vpr_setup *vpr_setup, OUTP t_arch *arch);
void vpr_pack(INP t_vpr_setup vpr_setup, INP t_arch arch);
void vpr_init_pre_place_and_route(INP t_vpr_setup vpr_setup, INP t_arch Arch);
void vpr_place_and_route(INP t_vpr_setup vpr_setup, INP t_arch arch);
void vpr_free_all(INOUTP t_arch Arch, INOUTP t_options options, INOUTP t_vpr_setup vpr_setup);

/* Display general info to user */
void vpr_print_title(void);
void vpr_print_usage(void);

#endif












