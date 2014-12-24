/**
 VPR is a CAD tool used to conduct FPGA architecture exploration.  It takes, as input, a technology-mapped netlist and a description of the FPGA architecture being investigated.  
 VPR then generates a packed, placed, and routed FPGA (in .net, .place, and .route files respectively) that implements the input netlist.
 
 This file is where VPR starts execution.

 Key files in VPR:
 1.  libarchfpga/physical_types.h - Data structures that define the properties of the FPGA architecture
 2.  vpr_types.h - Very major file that defines the core data structures used in VPR.  This includes detailed architecture information, user netlist data structures, and data structures that describe the mapping between those two.
 3.  globals.h - Defines the global variables used by VPR.
 */

#include <cstdio>
#include <cstring>
#include <ctime>
using namespace std;

#include "vpr_api.h"
#include "util.h" /* for CLOCKS_PER_SEC */
#include "path_delay.h" /* for timing_analysis_runtime */

/**
 * VPR program
 * Generate FPGA architecture given architecture description
 * Pack, place, and route circuit into FPGA architecture
 * Electrical timing analysis on results
 *
 * Overall steps
 * 1.  Initialization
 * 2.  Pack
 * 3.  Place-and-route and timing analysis
 * 4.  Clean up
 */

int main(int argc, char **argv) {
	t_options Options;
	t_arch Arch;
	t_vpr_setup vpr_setup;
	clock_t entire_flow_begin,entire_flow_end;

	entire_flow_begin = clock();

	try{
		/* Read options, architecture, and circuit netlist */
		vpr_init(argc, argv, &Options, &vpr_setup, &Arch);

		/* If the user requests packing, do packing */
		if (vpr_setup.PackerOpts.doPacking) {
			vpr_pack(vpr_setup, Arch);
		}

		if (vpr_setup.PlacerOpts.doPlacement || vpr_setup.RouterOpts.doRouting) {
			vpr_init_pre_place_and_route(vpr_setup, Arch);
			
			#ifdef INTERPOSER_BASED_ARCHITECTURE
			vpr_setup_interposer_cut_locations(Arch);
			#endif
			
			vpr_place_and_route(&vpr_setup, Arch);
		}

		if (vpr_setup.PowerOpts.do_power) {
			vpr_power_estimation(vpr_setup, Arch);
		}
	
		entire_flow_end = clock();

        vpr_printf_info("Timing analysis took %g seconds.\n", float(timing_analysis_runtime) / CLOCKS_PER_SEC);
		vpr_printf_info("The entire flow of VPR took %g seconds.\n", 
				(float)(entire_flow_end - entire_flow_begin) / CLOCKS_PER_SEC);
	
		/* free data structures */
		vpr_free_all(Arch, Options, vpr_setup);
	}
	catch(t_vpr_error* vpr_error){
		vpr_print_error(vpr_error);
	}
	
	/* Return 0 to single success to scripts */
	return 0;
}




