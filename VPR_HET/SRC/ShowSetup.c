#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "xml_arch.h"
#include "SetupVPR.h"

/******** Function Prototypes ********/

static void ShowPlacerOpts(IN t_options Options,
			   IN struct s_placer_opts PlacerOpts,
			   IN struct s_annealing_sched AnnealSched);
static void ShowOperation(IN enum e_operation Operation);
static void ShowRouterOpts(IN struct s_router_opts RouterOpts);
static void ShowAnnealSched(IN struct s_annealing_sched AnnealSched);
static void ShowRoutingArch(IN struct s_det_routing_arch RoutingArch);


/******** Function Implementations ********/

void
ShowSetup(IN t_options Options,
	  IN t_arch Arch,
	  IN boolean TimingEnabled,
	  IN enum e_operation Operation,
	  IN struct s_placer_opts PlacerOpts,
	  IN struct s_annealing_sched AnnealSched,
	  IN struct s_router_opts RouterOpts,
	  IN struct s_det_routing_arch RoutingArch,
	  IN t_segment_inf * Segments,
	  IN t_timing_inf Timing,
	  IN t_subblock_data Subblocks)
{
    int i, j, num_p_inputs, num_p_outputs;

    printf("Timing analysis: %s\n", (TimingEnabled ? "ON" : "OFF"));
    printf("\n");

    ShowOperation(Operation);
    ShowPlacerOpts(Options, PlacerOpts, AnnealSched);
    if((ROUTE_ONLY == Operation) || (PLACE_AND_ROUTE == Operation))
	{
	    ShowRouterOpts(RouterOpts);
	}
    else
	{
	    printf("Router: DISABLED\n\n");
	}

    if(DETAILED == RouterOpts.route_type)
	ShowRoutingArch(RoutingArch);

    printf("The circuit will be mapped into a %d x %d array of clbs.\n",
	   nx, ny);
    printf("\n");
    printf("Netlist num_nets:  %d\n", num_nets);
    printf("Netlist num_blocks:  %d\n", num_blocks);

    /* Count I/O input and output pads */
    num_p_inputs = 0;
    num_p_outputs = 0;
    for(i = 0; i < num_blocks; i++)
	{
	    if(block[i].type == IO_TYPE)
		{
		    for(j = 0; j < IO_TYPE->num_pins; j++)
			{
			    if(block[i].nets[j] != OPEN)
				{
				    if(IO_TYPE->
				       class_inf[IO_TYPE->pin_class[j]].
				       type == DRIVER)
					{
					    num_p_inputs++;
					}
				    else
					{
					    assert(IO_TYPE->
						   class_inf[IO_TYPE->
							     pin_class[j]].
						   type == RECEIVER);
					    num_p_outputs++;
					}
				}
			}
		}
	}

/* Print out each block separately instead */
    printf("Netlist inputs pins:  %d\n", num_p_inputs);
    printf("Netlist output pins:  %d\n", num_p_outputs);
    printf("\n");
}


static void
ShowRoutingArch(IN struct s_det_routing_arch RoutingArch)
{

    printf("RoutingArch.directionality:  ");
    switch (RoutingArch.directionality)
	{
	case BI_DIRECTIONAL:
	    printf("BI_DIRECTIONAL\n");
	    break;
	case UNI_DIRECTIONAL:
	    printf("UNI_DIRECTIONAL\n");
	    break;
	default:
	    printf("<Unknown>\n");
	    exit(1);
	}

    printf("RoutingArch.switch_block_type:  ");
    switch (RoutingArch.switch_block_type)
	{
	case SUBSET:
	    printf("SUBSET\n");
	    break;
	case WILTON:
	    printf("WILTON\n");
	    break;
	case UNIVERSAL:
	    printf("UNIVERSAL\n");
	    break;
	case FULL:
	    printf("FULL\n");
	    break;
	default:
	    printf("<Unknown>\n");
	    exit(1);
	}

    printf("RoutingArch.Fs:  %d\n", RoutingArch.Fs);

    printf("\n");
}


static void
ShowAnnealSched(IN struct s_annealing_sched AnnealSched)
{

    printf("AnnealSched.type:  ");
    switch (AnnealSched.type)
	{
	case AUTO_SCHED:
	    printf("AUTO_SCHED\n");
	    break;
	case USER_SCHED:
	    printf("USER_SCHED\n");
	    break;
	default:
	    printf("<Unknown>\n");
	    exit(1);
	}

    printf("AnnealSched.inner_num:  %f\n", AnnealSched.inner_num);

    if(USER_SCHED == AnnealSched.type)
	{
	    printf("AnnealSched.init_t:  %f\n", AnnealSched.init_t);
	    printf("AnnealSched.alpha_t:  %f\n", AnnealSched.alpha_t);
	    printf("AnnealSched.exit_t:  %f\n", AnnealSched.exit_t);
	}
}


static void
ShowRouterOpts(IN struct s_router_opts RouterOpts)
{

    printf("RouterOpts.route_type:  ");
    switch (RouterOpts.route_type)
	{
	case GLOBAL:
	    printf("GLOBAL\n");
	    break;
	case DETAILED:
	    printf("DETAILED\n");
	    break;
	default:
	    printf("<Unknown>\n");
	    exit(1);
	}

    if(DETAILED == RouterOpts.route_type)
	{
	    printf("RouterOpts.router_algorithm:  ");
	    switch (RouterOpts.router_algorithm)
		{
		case BREADTH_FIRST:
		    printf("BREADTH_FIRST\n");
		    break;
		case TIMING_DRIVEN:
		    printf("TIMING_DRIVEN\n");
		    break;
		case DIRECTED_SEARCH:
		    printf("DIRECTED_SEARCH\n");
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("RouterOpts.base_cost_type:  ");
	    switch (RouterOpts.base_cost_type)
		{
		case INTRINSIC_DELAY:
		    printf("INTRINSIC_DELAY\n");
		    break;
		case DELAY_NORMALIZED:
		    printf("DELAY_NORMALIZED\n");
		    break;
		case DEMAND_ONLY:
		    printf("DEMAND_ONLY\n");
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("RouterOpts.fixed_channel_width:  ");
	    if(NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width)
		{
		    printf("NO_FIXED_CHANNEL_WIDTH\n");
		}
	    else
		{
		    printf("%d\n", RouterOpts.fixed_channel_width);
		}

	    printf("RouterOpts.acc_fac:  %f\n", RouterOpts.acc_fac);
	    printf("RouterOpts.bb_factor:  %d\n", RouterOpts.bb_factor);
	    printf("RouterOpts.bend_cost:  %f\n", RouterOpts.bend_cost);
	    printf("RouterOpts.first_iter_pres_fac:  %f\n",
		   RouterOpts.first_iter_pres_fac);
	    printf("RouterOpts.initial_pres_fac:  %f\n",
		   RouterOpts.initial_pres_fac);
	    printf("RouterOpts.pres_fac_mult:  %f\n",
		   RouterOpts.pres_fac_mult);
	    printf("RouterOpts.max_router_iterations:  %d\n",
		   RouterOpts.max_router_iterations);

	    if(TIMING_DRIVEN == RouterOpts.router_algorithm)
		{
		    printf("RouterOpts.astar_fac:  %f\n",
			   RouterOpts.astar_fac);
		    printf("RouterOpts.criticality_exp:  %f\n",
			   RouterOpts.criticality_exp);
		    printf("RouterOpts.max_criticality:  %f\n",
			   RouterOpts.max_criticality);
		}
	}
    else
	{
	    assert(GLOBAL == RouterOpts.route_type);
	    
		printf("RouterOpts.router_algorithm:  ");
	    switch (RouterOpts.router_algorithm)
		{
		case BREADTH_FIRST:
		    printf("BREADTH_FIRST\n");
		    break;
		case TIMING_DRIVEN:
		    printf("TIMING_DRIVEN\n");
		    break;
		case DIRECTED_SEARCH:
		    printf("DIRECTED_SEARCH\n");
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("RouterOpts.base_cost_type:  ");
	    switch (RouterOpts.base_cost_type)
		{
		case INTRINSIC_DELAY:
		    printf("INTRINSIC_DELAY\n");
		    break;
		case DELAY_NORMALIZED:
		    printf("DELAY_NORMALIZED\n");
		    break;
		case DEMAND_ONLY:
		    printf("DEMAND_ONLY\n");
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("RouterOpts.fixed_channel_width:  ");
	    if(NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width)
		{
		    printf("NO_FIXED_CHANNEL_WIDTH\n");
		}
	    else
		{
		    printf("%d\n", RouterOpts.fixed_channel_width);
		}

	    printf("RouterOpts.acc_fac:  %f\n", RouterOpts.acc_fac);
	    printf("RouterOpts.bb_factor:  %d\n", RouterOpts.bb_factor);
	    printf("RouterOpts.bend_cost:  %f\n", RouterOpts.bend_cost);
	    printf("RouterOpts.first_iter_pres_fac:  %f\n",
		   RouterOpts.first_iter_pres_fac);
	    printf("RouterOpts.initial_pres_fac:  %f\n",
		   RouterOpts.initial_pres_fac);
	    printf("RouterOpts.pres_fac_mult:  %f\n",
		   RouterOpts.pres_fac_mult);
	    printf("RouterOpts.max_router_iterations:  %d\n",
		RouterOpts.max_router_iterations);
		if(TIMING_DRIVEN == RouterOpts.router_algorithm)
		{
		    printf("RouterOpts.astar_fac:  %f\n",
			   RouterOpts.astar_fac);
		    printf("RouterOpts.criticality_exp:  %f\n",
			   RouterOpts.criticality_exp);
		    printf("RouterOpts.max_criticality:  %f\n",
			   RouterOpts.max_criticality);
		}
	}
    printf("\n");
}


static void
ShowOperation(IN enum e_operation Operation)
{
    printf("Operation:  ");
    switch (Operation)
	{
	case PLACE_AND_ROUTE:
	    printf("PLACE_AND_ROUTE\n");
	    break;
	case PLACE_ONLY:
	    printf("PLACE_ONLY\n");
	    break;
	case ROUTE_ONLY:
	    printf("ROUTE_ONLY\n");
	    break;
	case TIMING_ANALYSIS_ONLY:
	    printf("TIMING_ANALYSIS_ONLY\n");
	    break;
	default:
	    printf("<Unknown>\n");
	    exit(1);
	}
    printf("\n");
}


static void
ShowPlacerOpts(IN t_options Options,
	       IN struct s_placer_opts PlacerOpts,
	       IN struct s_annealing_sched AnnealSched)
{

    printf("PlacerOpts.place_freq:  ");
    switch (PlacerOpts.place_freq)
	{
	case PLACE_ONCE:
	    printf("PLACE_ONCE\n");
	    break;
	case PLACE_ALWAYS:
	    printf("PLACE_ALWAYS\n");
	    break;
	case PLACE_NEVER:
	    printf("PLACE_NEVER\n");
	    break;
	default:
	    printf("<Unknown>\n");
	    exit(1);
	}
    if((PLACE_ONCE == PlacerOpts.place_freq) ||
       (PLACE_ALWAYS == PlacerOpts.place_freq))
	{

	    printf("PlacerOpts.place_algorithm:  ");
	    switch (PlacerOpts.place_algorithm)
		{
		case BOUNDING_BOX_PLACE:
		    printf("BOUNDING_BOX_PLACE\n");
		    break;
		case NET_TIMING_DRIVEN_PLACE:
		    printf("NET_TIMING_DRIVEN_PLACE\n");
		    break;
		case PATH_TIMING_DRIVEN_PLACE:
		    printf("PATH_TIMING_DRIVEN_PLACE\n");
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("PlacerOpts.place_cost_type:  ");
	    switch (PlacerOpts.place_cost_type)
		{
		case LINEAR_CONG:
		    printf("LINEAR_CONG\n");
		    break;
		case NONLINEAR_CONG:
		    printf("NONLINEAR_CONG\n");
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("PlacerOpts.pad_loc_type:  ");
	    switch (PlacerOpts.pad_loc_type)
		{
		case FREE:
		    printf("FREE\n");
		    break;
		case RANDOM:
		    printf("RANDOM\n");
		    break;
		case USER:
		    printf("USER '%s'\n", PlacerOpts.pad_loc_file);
		    break;
		default:
		    printf("<Unknown>\n");
		    exit(1);
		}

	    printf("PlacerOpts.place_cost_exp:  %f\n",
		   PlacerOpts.place_cost_exp);

	    if((LINEAR_CONG == PlacerOpts.place_cost_type) ||
	       (Options.Count[OT_PLACE_CHAN_WIDTH]))
		{
		    printf("PlacerOpts.place_chan_width:  %d\n",
			   PlacerOpts.place_chan_width);
		}

	    if(NONLINEAR_CONG == PlacerOpts.place_cost_type)
		{
		    printf("PlacerOpts.num_regions:  %d\n",
			   PlacerOpts.num_regions);
		}

	    if((NET_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm) ||
	       (PATH_TIMING_DRIVEN_PLACE == PlacerOpts.place_algorithm))
		{
		    printf("PlacerOpts.inner_loop_recompute_divider:  %d\n",
			   PlacerOpts.inner_loop_recompute_divider);
		    printf("PlacerOpts.recompute_crit_iter:  %d\n",
			   PlacerOpts.recompute_crit_iter);
		    printf("PlacerOpts.timing_tradeoff:  %f\n",
			   PlacerOpts.timing_tradeoff);
		    printf("PlacerOpts.td_place_exp_first:  %f\n",
			   PlacerOpts.td_place_exp_first);
		    printf("PlacerOpts.td_place_exp_last:  %f\n",
			   PlacerOpts.td_place_exp_last);
		}

	    printf("PlaceOpts.seed:  %d\n", PlacerOpts.seed);

	    ShowAnnealSched(AnnealSched);
	}
    printf("\n");

}
