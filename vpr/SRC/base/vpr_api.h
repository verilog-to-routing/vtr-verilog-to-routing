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
#include "OptionTokens.h"
#include "globals.h"
#include "util.h"
#include "read_xml_arch_file.h"
#include "vpr_utils.h"
#include "place_macro.h"

/* Main VPR Operations */
void vpr_init(INP int argc, INP char **argv, OUTP t_options *options,
		OUTP t_vpr_setup *vpr_setup, OUTP t_arch *arch);
void vpr_pack(INP t_vpr_setup vpr_setup, INP t_arch arch);
void vpr_init_pre_place_and_route(INP t_vpr_setup vpr_setup, INP t_arch Arch);
boolean vpr_place_and_route(INP t_vpr_setup vpr_setup, INP t_arch arch);
void vpr_power_estimation(t_vpr_setup vpr_setup, t_arch Arch);
void vpr_free_vpr_data_structures(INOUTP t_arch Arch, INOUTP t_options options,
		INOUTP t_vpr_setup vpr_setup);
void vpr_free_all(INOUTP t_arch Arch, INOUTP t_options options,
		INOUTP t_vpr_setup vpr_setup);

/* Display general info to user */
void vpr_print_title(void);
void vpr_print_usage(void);

/****************************************************************************************************
 * Advanced functions
 *  Used when you need fine-grained control over VPR that the main VPR operations do not enable
 ****************************************************************************************************/
/* Read in user options */
void vpr_read_options(INP int argc, INP char **argv, OUTP t_options * options);
/* Read in arch and circuit */
void vpr_setup_vpr(INP t_options *Options, INP boolean TimingEnabled,
		INP boolean readArchFile, OUTP struct s_file_name_opts *FileNameOpts,
		INOUTP t_arch * Arch, OUTP enum e_operation *Operation,
		OUTP t_model ** user_models, OUTP t_model ** library_models,
		OUTP struct s_packer_opts *PackerOpts,
		OUTP struct s_placer_opts *PlacerOpts,
		OUTP struct s_annealing_sched *AnnealSched,
		OUTP struct s_router_opts *RouterOpts,
		OUTP struct s_det_routing_arch *RoutingArch,
		OUTP vector <t_lb_type_rr_node> **PackerRRGraph,
		OUTP t_segment_inf ** Segments, OUTP t_timing_inf * Timing,
		OUTP boolean * ShowGraphics, OUTP int *GraphPause,
		t_power_opts * PowerOpts);
/* Check inputs are reasonable */
void vpr_check_options(INP t_options Options, INP boolean TimingEnabled);
void vpr_check_arch(INP t_arch Arch, INP boolean TimingEnabled);
/* Verify settings don't conflict or otherwise not make sense */
void vpr_check_setup(INP enum e_operation Operation,
		INP struct s_placer_opts PlacerOpts,
		INP struct s_annealing_sched AnnealSched,
		INP struct s_router_opts RouterOpts,
		INP struct s_det_routing_arch RoutingArch, INP t_segment_inf * Segments,
		INP t_timing_inf Timing, INP t_chan_width_dist Chans);
/* Read blif file and sweep unused components */
void vpr_read_and_process_blif(INP char *blif_file,
		INP boolean sweep_hanging_nets_and_inputs, INP t_model *user_models,
		INP t_model *library_models, boolean read_activity_file,
		char * activity_file);
/* Show current setup */
void vpr_show_setup(INP t_options options, INP t_vpr_setup vpr_setup);

/* Output file names management */
void vpr_alloc_and_load_output_file_names(const char* default_name);
void vpr_set_output_file_name(enum e_output_files ename, const char *name,
		const char* default_name);
char *vpr_get_output_file_name(enum e_output_files ename);

/* resync netlists */
t_trace* vpr_resync_post_route_netlist_to_TI_CLAY_v1_architecture(
		INP boolean apply_logical_equivalence_handling,
		INP const t_arch *arch);

/* Prints user file or internal errors for VPR */
void vpr_print_error(t_vpr_error* vpr_error);

#ifdef INTERPOSER_BASED_ARCHITECTURE
void vpr_setup_interposer_cut_locations(t_arch Arch);
#endif

#endif
