#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"

/* Checks that options don't conflict and that 
 * options aren't specified that may conflict */
void CheckOptions(INP t_options Options, INP boolean TimingEnabled) {
	boolean TimingPlacer;
	boolean TimingRouter;
	boolean default_flow;

	const struct s_TokenPair *Cur;
	enum e_OptionBaseToken Yes;

	default_flow = (boolean) (Options.Count[OT_ROUTE] == 0
			&& Options.Count[OT_PLACE] == 0 && Options.Count[OT_PACK] == 0
			&& Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY] == 0);

	/* Check that all filenames were given */
	if ((NULL == Options.CircuitName) || (NULL == Options.ArchFile)) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"Not enough args. Need at least 'vpr <archfile> <circuit_name>'.\n");
	}

	/* Check that options aren't over specified */
	Cur = OptionBaseTokenList;
	while (Cur->Str) {
		if (Options.Count[Cur->Enum] > 1) {
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
					"Parameter '%s' was specified more than once on command line.\n", Cur->Str);
		}
		++Cur;
	}

	/* Todo: Add in checks for packer   */

	/* Check for conflicting parameters and determine if placer and 
	 * router are on. */

	if (Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY]
			&& (Options.Count[OT_PACK] || Options.Count[OT_PLACE]
					|| Options.Count[OT_ROUTE])) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
				"'cluster'/'route'/'place', and 'timing_analysis_only_with_net_delay' are mutually exclusive flags..\n");
	}

	/* If placing and timing is enabled, default to a timing placer */
	TimingPlacer = (boolean)((Options.Count[OT_PLACE] || default_flow) && TimingEnabled);
	if (Options.Count[OT_PLACE_ALGORITHM] > 0) {
		if ((PATH_TIMING_DRIVEN_PLACE != Options.PlaceAlgorithm)
				&& (NET_TIMING_DRIVEN_PLACE != Options.PlaceAlgorithm)) {
			/* Turn off the timing placer if they request a different placer */
			TimingPlacer = FALSE;
		}
	}

	/* If routing and timing is enabled, default to a timing router */
	TimingRouter = (boolean)((Options.Count[OT_ROUTE] || default_flow) && TimingEnabled);
	if (Options.Count[OT_ROUTER_ALGORITHM] > 0) {
		if (TIMING_DRIVEN != Options.RouterAlgorithm) {
			/* Turn off the timing router if they request a different router */
			TimingRouter = FALSE;
		}
	}

	Yes = OT_BASE_UNKNOWN;
	if (Options.Count[OT_SEED] > 0) {
		Yes = OT_SEED;
	}
	if (Options.Count[OT_INNER_NUM] > 0) {
		Yes = OT_INNER_NUM;
	}
	if (Options.Count[OT_INIT_T] > 0) {
		Yes = OT_INIT_T;
	}
	if (Options.Count[OT_ALPHA_T] > 0) {
		Yes = OT_ALPHA_T;
	}
	if (Options.Count[OT_EXIT_T] > 0) {
		Yes = OT_EXIT_T;
	}
	if (Options.Count[OT_FIX_PINS] > 0) {
		Yes = OT_FIX_PINS;
	}
	if (Options.Count[OT_PLACE_ALGORITHM] > 0) {
		Yes = OT_PLACE_ALGORITHM;
	}
	if (Options.Count[OT_PLACE_COST_EXP] > 0) {
		Yes = OT_PLACE_COST_EXP;
	}
	if (Options.Count[OT_PLACE_CHAN_WIDTH] > 0) {
		Yes = OT_PLACE_CHAN_WIDTH;
	}
	if (Options.Count[OT_ENABLE_TIMING_COMPUTATIONS] > 0) {
		Yes = OT_ENABLE_TIMING_COMPUTATIONS;
	}
	if (Options.Count[OT_BLOCK_DIST] > 0) {
		Yes = OT_BLOCK_DIST;
	}
	/* Make sure if place is off none of those options were given */
	if ((Options.Count[OT_PLACE] == 0) && !default_flow
			&& (Yes < OT_BASE_UNKNOWN)) {
		Cur = OptionBaseTokenList;
		while (Cur->Str) {
			if (Yes == Cur->Enum) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
						"Option '%s' is not allowed when placement is not run.\n", Cur->Str);
			}
			++Cur;
		}
	}

	Yes = OT_BASE_UNKNOWN;
	if (Options.Count[OT_TIMING_TRADEOFF] > 0) {
		Yes = OT_TIMING_TRADEOFF;
	}
	if (Options.Count[OT_RECOMPUTE_CRIT_ITER] > 0) {
		Yes = OT_RECOMPUTE_CRIT_ITER;
	}
	if (Options.Count[OT_INNER_LOOP_RECOMPUTE_DIVIDER] > 0) {
		Yes = OT_INNER_LOOP_RECOMPUTE_DIVIDER;
	}
	if (Options.Count[OT_TD_PLACE_EXP_FIRST] > 0) {
		Yes = OT_TD_PLACE_EXP_FIRST;
	}
	if (Options.Count[OT_TD_PLACE_EXP_LAST] > 0) {
		Yes = OT_TD_PLACE_EXP_LAST;
	}
	/* Make sure if place is off none of those options were given */
	if ((FALSE == TimingPlacer) && (Yes < OT_BASE_UNKNOWN)) {
		Cur = OptionBaseTokenList;
		while (Cur->Str) {
			if (Yes == Cur->Enum) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
						"Option '%s' is not allowed when timing placement is not used.\n", Cur->Str);
			}
			++Cur;
		}
	}

	Yes = OT_BASE_UNKNOWN;
	if (Options.Count[OT_ROUTE_TYPE] > 0) {
		Yes = OT_ROUTE_TYPE;
	}
	if (Options.Count[OT_ROUTE_CHAN_WIDTH] > 0) {
		Yes = OT_ROUTE_CHAN_WIDTH;
	}
	if (Options.Count[OT_ROUTER_ALGORITHM] > 0) {
		Yes = OT_ROUTER_ALGORITHM;
	}
	if (Options.Count[OT_MAX_ROUTER_ITERATIONS] > 0) {
		Yes = OT_MAX_ROUTER_ITERATIONS;
	}
	if (Options.Count[OT_INITIAL_PRES_FAC] > 0) {
		Yes = OT_INITIAL_PRES_FAC;
	}
	if (Options.Count[OT_FIRST_ITER_PRES_FAC] > 0) {
		Yes = OT_FIRST_ITER_PRES_FAC;
	}
	if (Options.Count[OT_PRES_FAC_MULT] > 0) {
		Yes = OT_PRES_FAC_MULT;
	}
	if (Options.Count[OT_ACC_FAC] > 0) {
		Yes = OT_ACC_FAC;
	}
	if (Options.Count[OT_BB_FACTOR] > 0) {
		Yes = OT_BB_FACTOR;
	}
	if (Options.Count[OT_BASE_COST_TYPE] > 0) {
		Yes = OT_BASE_COST_TYPE;
	}
	if (Options.Count[OT_BEND_COST] > 0) {
		Yes = OT_BEND_COST;
	}
	if (Options.Count[OT_BASE_COST_TYPE] > 0) {
		Yes = OT_BASE_COST_TYPE;
	}
	if (Options.Count[OT_ASTAR_FAC] > 0) {
		Yes = OT_ASTAR_FAC;
	}
	Yes = OT_BASE_UNKNOWN;
	if (Options.Count[OT_MAX_CRITICALITY] > 0) {
		Yes = OT_MAX_CRITICALITY;
	}
	if (Options.Count[OT_CRITICALITY_EXP] > 0) {
		Yes = OT_CRITICALITY_EXP;
	}
	/* Make sure if timing router is off none of those options were given */
	if ((FALSE == TimingRouter) && (Yes < OT_BASE_UNKNOWN)) {
		Cur = OptionBaseTokenList;
		while (Cur->Str) {
			if (Yes == Cur->Enum) {
				vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
						"Option '%s' is not allowed when timing router is not used.\n", Cur->Str);
			}
			++Cur;
		}
	}
}
