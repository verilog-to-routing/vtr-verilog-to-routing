#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "CheckOptions.h"

void check_for_stage_options(const t_options& options, e_OptionBaseToken start, e_OptionBaseToken end, const char* stage_name);

void CheckOptions(const t_options& options, bool timing_enabled) {
    //By default we run all stages
    bool default_flow = (options.Count[OT_PACK] == 0 &&
                         options.Count[OT_PLACE] == 0 &&
                         options.Count[OT_ROUTE] == 0 &&
                         options.Count[OT_ANALYSIS] == 0);

    //By default we do timing-driven compilation
    bool timing_place = timing_enabled;
    bool timing_route = timing_enabled;

	/* Check that all filenames were given */
	if ((NULL == options.CircuitName) || (NULL == options.ArchFile)) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"Not enough args. Need at least 'vpr <archfile> <circuit_name>'.\n");
	}

	/* Check that options aren't over specified */
	const t_TokenPair* Cur = g_OptionBaseTokenList;
	while (Cur->Str) {
		if (options.Count[Cur->Enum] > 1) {
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
					"Parameter '%s' was specified more than once on command line.\n", Cur->Str);
		}
		++Cur;
	}

	/* Should we fall back to non-timing driven placement? */
    if(options.Count[OT_PLACE_ALGORITHM] && options.PlaceAlgorithm != PATH_TIMING_DRIVEN_PLACE) {
        timing_place = false;
    }

	/* Should we fall back to non-timing driven routing? */
    if(options.Count[OT_ROUTER_ALGORITHM] && options.RouterAlgorithm != TIMING_DRIVEN) {
        timing_route = false;
    }

	/* Check for conflicting parameters */

	/* If a dump of routing resource structs was requested then routing should have been specified at a
	   fixed channel width */
	if (options.Count[OT_DUMP_RR_STRUCTS_FILE] && !options.Count[OT_ROUTE_CHAN_WIDTH]){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"-route_chan_width option must be specified if dumping rr structs is requested (-dump_rr_structs_file option)\n");
	}
        /* If reading an external XML RR graph, routing should have a specified channel width */
	if (options.Count[OT_READ_RR_GRAPH] && !options.Count[OT_ROUTE_CHAN_WIDTH]){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"-route_chan_width option must be specified if reading an rr graph is requested (-read_rr_graph option)\n");
	}

    /* Warn about stage related parameters that are being specified when the stage is not going to run */

    if(!default_flow && !options.Count[OT_PACK]) {
        //We are not performing packing, so check for any related options
        check_for_stage_options(options,
                                OT__START_PACK_OPTIONS, 
                                OT__END_PACK_OPTIONS, 
                                "packing");
    } else if(!timing_place) {
        //We are doing packing, but in non-timing-driven mode, so check for any
        //timing driven packing options
        check_for_stage_options(options, 
                                OT__START_TIMING_PLACE_OPTIONS, 
                                OT__END_TIMING_PLACE_OPTIONS, 
                                "timing-driven packing");
    }

    if(!default_flow && !options.Count[OT_PLACE]) {
        //We are not performing placement, so check for any related options
        check_for_stage_options(options, 
                                OT__START_PLACE_OPTIONS, 
                                OT__END_PLACE_OPTIONS, 
                                "placement");
    } else if(!timing_place) {
        //We are doing placement, but in non-timing-driven mode, so check for any
        //timing driven placment options
        check_for_stage_options(options, 
                                OT__START_TIMING_PLACE_OPTIONS, 
                                OT__END_TIMING_PLACE_OPTIONS, 
                                "timing-driven placement");
    }

    if(!default_flow && !options.Count[OT_ROUTE]) {
        //We are not performing routing, so check for any related options
        check_for_stage_options(options, 
                                OT__START_ROUTE_OPTIONS, 
                                OT__END_ROUTE_OPTIONS,
                                "routing");
    } else if(!timing_route) {
        //We are doing routing, but in non-timing-driven mode, so check for any
        //timing driven routing options
        check_for_stage_options(options, 
                                OT__START_TIMING_ROUTE_OPTIONS, 
                                OT__END_TIMING_ROUTE_OPTIONS,
                                "timing-driven routing");
    }

    if(!default_flow && !options.Count[OT_ANALYSIS] && !options.Count[OT_ROUTE]) {
        //We are not performing analysis, so check for any related options
        // Note that by default analysis runs if routing runs
        check_for_stage_options(options, 
                                OT__START_ANALYSIS_OPTIONS, 
                                OT__END_ANALYSIS_OPTIONS, 
                                "analysis");

    }

    if(!options.Count[OT_POWER]) {
        //We are not performing power analysis, so check for any related options
        check_for_stage_options(options, 
                                OT__START_ANALYSIS_POWER_OPTIONS, 
                                OT__END_ANALYSIS_POWER_OPTIONS, 
                                "analysis power estimation");
    }

}

void check_for_stage_options(const t_options& options, e_OptionBaseToken start, e_OptionBaseToken end, const char* stage_name) {

    //The start and end are special markers in e_OptionBaseToken
    //which should never be specified
    VTR_ASSERT(options.Count[start] == 0);
    VTR_ASSERT(options.Count[end] == 0);

    //Walk through all possible options and see if any in the exclusive interval (start, end)
    //have been set
    t_TokenPair* Cur = g_OptionBaseTokenList;
    while (Cur->Str) {
        auto enum_val = Cur->Enum;

        if (options.Count[enum_val] != 0 && enum_val > start && enum_val < end) {
            //The option was specified and falls within the range of options
            //
            //Note that in e_OptionBaseToken the special OT__START* and OT__END*
            //values bracket the valid options, so we do not include those value in
            //the check

            //Warn the user that the specified option will not do anything
            vtr::printf_warning(__FILE__, __LINE__, 
                                "Command-line option '%s' has no effect since %s will not run.\n", Cur->Str, stage_name);
        }
        ++Cur;
    }
}
