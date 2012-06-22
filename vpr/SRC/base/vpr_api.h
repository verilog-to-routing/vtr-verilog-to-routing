/**
General API for VPR 
Other software tools should generally call just the functions defined here
For advanced/power users, you can call functions defined elsewhere in VPR or modify the data structures directly at your discretion but be aware that doing so can break the correctness of VPR

 General Usage: 
   1. vpr_read_options and vpr_read_architecture_file
   2. vpr_pack
   3. vpr_init_place_and_route_arch
   4. vpr_place_and_route
   5. free data structures when you are done

Author: Jason Luu
June 21, 2012
*/

#ifndef VPR_API_H
#define VPR_API_H

/* Display general info to user */
void vpr_print_title(void);
void vpr_print_usage(void);

/* Run VPR Operations */
void vpr_init_options_and_arch(INP t_options *Options, OUTP t_vpr_settings *settings, OUTP t_arch *Arch);
void vpr_init_place_and_route_arch(INP t_arch Arch);

/* Free internal data structures */
void vpr_free_arch(t_arch* Arch);
void vpr_free_options(t_options *options);
void vpr_free_complex_block_types(void);

#endif












