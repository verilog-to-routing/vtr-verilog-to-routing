/**
General API for VPR 
Other software tools should generally call just the functions defined here
For advanced/power users, you can call functions defined elsewhere in VPR or modify the data structures directly at your discretion but be aware that doing so can break the correctness of VPR

 General Usage: 
   1. vpr_init
   2. vpr_pack
   3. vpr_init_pre_place_and_route
   4. vpr_place_and_route
   5. vpr_free_all

Author: Jason Luu
June 21, 2012
*/

#ifndef VPR_API_H
#define VPR_API_H

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












