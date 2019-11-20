#include <assert.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "check_netlist.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "read_netlist.h"
#include "globals.h"
#include "xml_arch.h"
#include "SetupVPR.h"

static void SetupOperation(IN t_options Options,
			   OUT enum e_operation *Operation);
static void SetupPlacerOpts(IN t_options Options,
			    IN boolean TimingEnabled,
			    OUT struct s_placer_opts *PlacerOpts);
static void SetupAnnealSched(IN t_options Options,
			     OUT struct s_annealing_sched *AnnealSched);
static void SetupRouterOpts(IN t_options Options,
			    IN boolean TimingEnabled,
			    OUT struct s_router_opts *RouterOpts);
static void SetupGlobalRoutingArch(OUT struct s_det_routing_arch *RoutingArch,
				   OUT t_segment_inf ** Segments);
static void SetupRoutingArch(IN t_arch Arch,
			     OUT struct s_det_routing_arch *RoutingArch);
static void SetupTiming(IN t_options Options,
			IN t_arch Arch,
			IN boolean TimingEnabled,
			IN enum e_operation Operation,
			IN struct s_placer_opts PlacerOpts,
			IN struct s_router_opts RouterOpts,
			OUT t_timing_inf * Timing);
static void load_subblock_info_to_type(INOUT t_subblock_data * subblocks,
				       INOUT t_type_ptr type);
static void InitArch(IN t_arch Arch);
static void alloc_and_load_grid(INOUT int *num_instances_type);	/* [0..num_types-1] */
static void freeGrid();
static void CheckGrid(void);
static t_type_ptr find_type_col(IN int x);
static void SetupSwitches(IN t_arch Arch,
			  INOUT struct s_det_routing_arch *RoutingArch,
			  IN struct s_switch_inf *ArchSwitches,
			  IN int NumArchSwitches);

/* Sets VPR parameters and defaults. Does not do any error checking
 * as this should have been done by the various input checkers */
void
SetupVPR(IN t_options Options,
	 IN boolean TimingEnabled,
	 OUT t_arch * Arch,
	 OUT enum e_operation *Operation,
	 OUT struct s_placer_opts *PlacerOpts,
	 OUT struct s_annealing_sched *AnnealSched,
	 OUT struct s_router_opts *RouterOpts,
	 OUT struct s_det_routing_arch *RoutingArch,
	 OUT t_segment_inf ** Segments,
	 OUT t_timing_inf * Timing,
	 OUT t_subblock_data * Subblocks,
	 OUT boolean * ShowGraphics,
	 OUT int *GraphPause)
{

    SetupOperation(Options, Operation);
    SetupPlacerOpts(Options, TimingEnabled, PlacerOpts);
    SetupAnnealSched(Options, AnnealSched);
    SetupRouterOpts(Options, TimingEnabled, RouterOpts);

    XmlReadArch(Options.ArchFile, TimingEnabled, Arch,
		&type_descriptors, &num_types);

    *Segments = Arch->Segments;
    RoutingArch->num_segment = Arch->num_segments;

    SetupSwitches(*Arch, RoutingArch, Arch->Switches, Arch->num_switches);
    SetupRoutingArch(*Arch, RoutingArch);
    SetupTiming(Options, *Arch, TimingEnabled, *Operation,
		*PlacerOpts, *RouterOpts, Timing);

    /* init global variables */
    OutFilePrefix = Options.OutFilePrefix;
    grid_logic_tile_area = Arch->grid_logic_tile_area;
    ipin_mux_trans_size = Arch->ipin_mux_trans_size;

    /* Set seed for pseudo-random placement, default seed to 1 */
    PlacerOpts->seed = 1;
    if(Options.Count[OT_SEED])
	{
	    PlacerOpts->seed = Options.Seed;
	}
    my_srandom(PlacerOpts->seed);

    *GraphPause = 1;		/* DEFAULT */
    if(Options.Count[OT_AUTO])
	{
	    *GraphPause = Options.GraphPause;
	}
#ifdef NO_GRAPHICS
    *ShowGraphics = FALSE;	/* DEFAULT */
#else /* NO_GRAPHICS */
    *ShowGraphics = TRUE;	/* DEFAULT */
    if(Options.Count[OT_NODISP])
	{
	    *ShowGraphics = FALSE;
	}
#endif /* NO_GRAPHICS */

#ifdef CREATE_ECHO_FILES
    EchoArch("arch.echo", type_descriptors, num_types);
#endif

    if(Options.NetFile)
	{
	    read_netlist(Options.NetFile, num_types, type_descriptors,
			 IO_TYPE, 0, 1, Subblocks, &num_blocks, &block,
			 &num_nets, &net);
	    /* This is done so that all blocks have subblocks and can be treated the same */
	    load_subblock_info_to_type(Subblocks, IO_TYPE);
	    check_netlist(Subblocks);
	}

    InitArch(*Arch);
}

/* This is a modification of the init_arch function to use Arch as param.
 * Sets globals: nx, ny
 * Allocs globals: chan_width_x, chan_width_y, grid
 * Depends on num_clbs, pins_per_clb */
static void
InitArch(IN t_arch Arch)
{
    int *num_instances_type, *num_blocks_type;
    int i;
    int current, high, low;
    boolean fit;

    current = nint(sqrt(num_blocks));	/* current is the value of the smaller side of the FPGA */
    low = 1;
    high = -1;

    num_instances_type = my_calloc(num_types, sizeof(int));
    num_blocks_type = my_calloc(num_types, sizeof(int));

    for(i = 0; i < num_blocks; i++)
	{
	    num_blocks_type[block[i].type->index]++;
	}

    if(Arch.clb_grid.IsAuto)
	{
	    /* Auto-size FPGA, perform a binary search */
	    while(high == -1 || low < high)
		{
		    /* Generate grid */
		    if(Arch.clb_grid.Aspect >= 1.0)
			{
			    ny = current;
			    nx = nint(current * Arch.clb_grid.Aspect);
			}
		    else
			{
			    nx = current;
			    ny = nint(current / Arch.clb_grid.Aspect);
			}
#if DEBUG
		    printf("Auto-sizing FPGA, try x = %d y = %d\n", nx, ny);
#endif
		    alloc_and_load_grid(num_instances_type);
		    freeGrid();

		    /* Test if netlist fits in grid */
		    fit = TRUE;
		    for(i = 0; i < num_types; i++)
			{
			    if(num_blocks_type[i] > num_instances_type[i])
				{
				    fit = FALSE;
				    break;
				}
			}

		    /* get next value */
		    if(!fit)
			{
			    /* increase size of max */
			    if(high == -1)
				{
				    current = current * 2;
				    if(current > MAX_SHORT)
					{
					    printf(ERRTAG
						   "FPGA required is too large for current architecture settings\n");
					    exit(1);
					}
				}
			    else
				{
				    if(low == current)
					current++;
				    low = current;
				    current = low + ((high - low) / 2);
				}
			}
		    else
			{
			    high = current;
			    current = low + ((high - low) / 2);
			}
		}
	    /* Generate grid */
	    if(Arch.clb_grid.Aspect >= 1.0)
		{
		    ny = current;
		    nx = nint(current * Arch.clb_grid.Aspect);
		}
	    else
		{
		    nx = current;
		    ny = nint(current / Arch.clb_grid.Aspect);
		}
	    alloc_and_load_grid(num_instances_type);
	    printf("FPGA auto-sized to, x = %d y = %d\n", nx, ny);
	}
    else
	{
	    nx = Arch.clb_grid.W;
	    ny = Arch.clb_grid.H;
	    alloc_and_load_grid(num_instances_type);
	}

    /* Test if netlist fits in grid */
    fit = TRUE;
    for(i = 0; i < num_types; i++)
	{
	    if(num_blocks_type[i] > num_instances_type[i])
		{
		    fit = FALSE;
		    break;
		}
	}
    if(!fit)
	{
	    printf(ERRTAG "Not enough physical locations for type %s, "
		   "number of blocks is %d but number of locations is %d\n",
		   num_blocks_type[i], num_instances_type[i]);
	    exit(1);
	}

	printf("\nResource Usage:\n");
	for(i = 0; i < num_types; i++)
	{
		printf("Netlist      %d\tblocks of type %s\n", 
			num_blocks_type[i], type_descriptors[i].name);
		printf("Architecture %d\tblocks of type %s\n", 
			num_instances_type[i], type_descriptors[i].name);
	}
	printf("\n");
    chan_width_x = (int *)my_malloc((ny + 1) * sizeof(int));
    chan_width_y = (int *)my_malloc((nx + 1) * sizeof(int));

    free(num_blocks_type);
    free(num_instances_type);
}


/* Create and fill FPGA architecture grid.         */
static void
alloc_and_load_grid(INOUT int *num_instances_type)
{

    int i, j;
    t_type_ptr type;

#ifdef SHOW_ARCH
    FILE *dump;
#endif

    /* If both nx and ny are 1, we only have one valid location for a clb. *
     * That's a major problem, as won't be able to move the clb and the    *
     * find_to routine that tries moves in the placer will go into an      *
     * infinite loop trying to move it.  Exit with an error message        *
     * instead.                                                            */

    if((nx == 1) && (ny == 1) && (num_blocks > 0))
	{
	    printf("Error:\n");
	    printf
		("Sorry, can't place a circuit with only one valid location\n");
	    printf("for a logic block (clb).\n");
	    printf("Try me with a more realistic circuit!\n");
	    exit(1);
	}

    /* To remove this limitation, change ylow etc. in t_rr_node to        *
     * * be ints instead.  Used shorts to save memory.                      */
    if((nx > 32766) || (ny > 32766))
	{
	    printf("Error:  nx and ny must be less than 32767, since the \n");
	    printf("router uses shorts (16-bit) to store coordinates.\n");
	    printf("nx: %d.  ny: %d.\n", nx, ny);
	    exit(1);
	}

    assert(nx >= 1 && ny >= 1);

    grid = (struct s_grid_tile **)alloc_matrix(0, (nx + 1),
					       0, (ny + 1),
					       sizeof(struct s_grid_tile));

    /* Clear the full grid to have no type (NULL), no capacity, etc */
    for(i = 0; i <= (nx + 1); ++i)
	{
	    for(j = 0; j <= (ny + 1); ++j)
		{
		    memset(&grid[i][j], 0, (sizeof(struct s_grid_tile)));
		}
	}

    for(i = 0; i < num_types; i++)
	{
	    num_instances_type[i] = 0;
	}

    /* Nothing goes in the corners. */
    grid[0][0].type = grid[nx + 1][0].type = EMPTY_TYPE;
    grid[0][ny + 1].type = grid[nx + 1][ny + 1].type = EMPTY_TYPE;
    num_instances_type[EMPTY_TYPE->index] = 4;

    for(i = 1; i <= nx; i++)
	{
	    grid[i][0].blocks =
		(int *)my_malloc(sizeof(int) * IO_TYPE->capacity);
	    grid[i][0].type = IO_TYPE;

	    grid[i][ny + 1].blocks =
		(int *)my_malloc(sizeof(int) * IO_TYPE->capacity);
	    grid[i][ny + 1].type = IO_TYPE;

	    for(j = 0; j < IO_TYPE->capacity; j++)
		{
		    grid[i][0].blocks[j] = EMPTY;
		    grid[i][ny + 1].blocks[j] = EMPTY;
		}
	}

    for(i = 1; i <= ny; i++)
	{
	    grid[0][i].blocks =
		(int *)my_malloc(sizeof(int) * IO_TYPE->capacity);
	    grid[0][i].type = IO_TYPE;

	    grid[nx + 1][i].blocks =
		(int *)my_malloc(sizeof(int) * IO_TYPE->capacity);
	    grid[nx + 1][i].type = IO_TYPE;
	    for(j = 0; j < IO_TYPE->capacity; j++)
		{
		    grid[0][i].blocks[j] = EMPTY;
		    grid[nx + 1][i].blocks[j] = EMPTY;
		}
	}

    num_instances_type[IO_TYPE->index] = 2 * IO_TYPE->capacity * (nx + ny);

    for(i = 1; i <= nx; i++)
	{			/* Interior (LUT) cells */
	    type = find_type_col(i);
	    for(j = 1; j <= ny; j++)
		{
		    grid[i][j].type = type;
		    grid[i][j].offset = (j - 1) % type->height;
		    if(j + grid[i][j].type->height - 1 - grid[i][j].offset >
		       ny)
			{
			    grid[i][j].type = EMPTY_TYPE;
			    grid[i][j].offset = 0;
			}

		    if(type->capacity > 1)
			{
			    printf(ERRTAG
				   "In FillArch() expected core blocks to have capacity <= 1 but "
				   "(%d, %d) has type '%s' and capacity %d\n",
				   i, j, grid[i][j].type->name,
				   grid[i][j].type->capacity);
			    exit(1);
			}

		    grid[i][j].blocks = (int *)my_malloc(sizeof(int));
		    grid[i][j].blocks[0] = EMPTY;
		    if(grid[i][j].offset == 0)
			{
			    num_instances_type[grid[i][j].type->index]++;
			}
		}
	}

    CheckGrid();

#ifdef SHOW_ARCH
    /* DEBUG code */
    dump = my_fopen("grid_type_dump.txt", "w");
    for(j = (ny + 1); j >= 0; --j)
	{
	    for(i = 0; i <= (nx + 1); ++i)
		{
		    fprintf(dump, "%c", grid[i][j].type->name[1]);
		}
	    fprintf(dump, "\n");
	}
    fclose(dump);
#endif
}

static void
freeGrid()
{
    int i, j;

    for(i = 0; i <= (nx + 1); ++i)
	{
	    for(j = 0; j <= (ny + 1); ++j)
		{
		    free(grid[i][j].blocks);
		}
	}
    free_matrix(grid, 0, nx + 1, 0, sizeof(struct s_grid_tile));
}


static void
CheckGrid()
{
    int i, j;

    /* Check grid is valid */
    for(i = 0; i <= (nx + 1); ++i)
	{
	    for(j = 0; j <= (ny + 1); ++j)
		{
		    if(NULL == grid[i][j].type)
			{
			    printf(ERRTAG "grid[%d][%d] has no type.\n", i,
				   j);
			    exit(1);
			}

		    if(grid[i][j].usage != 0)
			{
			    printf(ERRTAG
				   "grid[%d][%d] has non-zero usage (%d) "
				   "before netlist load.\n", i, j,
				   grid[i][j].usage);
			    exit(1);
			}

		    if((grid[i][j].offset < 0) ||
		       (grid[i][j].offset >= grid[i][j].type->height))
			{
			    printf(ERRTAG
				   "grid[%d][%d] has invalid offset (%d)\n",
				   i, j, grid[i][j].offset);
			    exit(1);
			}

		    if((NULL == grid[i][j].blocks) &&
		       (grid[i][j].type->capacity > 0))
			{
			    printf(ERRTAG
				   "grid[%d][%d] has no block list allocated.\n",
				   i, j);
			    exit(1);
			}
		}
	}
}

static t_type_ptr
find_type_col(IN int x)
{
    int i, j;
    int start, repeat;
    float rel;
    boolean match;
    int priority, num_loc;
    t_type_ptr column_type;

    priority = FILL_TYPE->grid_loc_def[0].priority;
    column_type = FILL_TYPE;

    for(i = 0; i < num_types; i++)
	{
	    if(&type_descriptors[i] == IO_TYPE ||
	       &type_descriptors[i] == EMPTY_TYPE ||
	       &type_descriptors[i] == FILL_TYPE)
		continue;
	    num_loc = type_descriptors[i].num_grid_loc_def;
	    for(j = 0; j < num_loc; j++)
		{
		    if(priority <
		       type_descriptors[i].grid_loc_def[j].priority)
			{
			    match = FALSE;
			    if(type_descriptors[i].grid_loc_def[j].
			       grid_loc_type == COL_REPEAT)
				{
				    start =
					type_descriptors[i].grid_loc_def[j].
					start_col;
				    repeat =
					type_descriptors[i].grid_loc_def[j].
					repeat;
				    if(start < 0)
					{
					    start += (nx + 1);
					}
				    if(x == start)
					{
					    match = TRUE;
					}
				    else if(repeat > 0 && x > start
					    && start > 0)
					{
					    if((x - start) % repeat == 0)
						{
						    match = TRUE;
						}
					}
				}
			    else if(type_descriptors[i].grid_loc_def[j].
				    grid_loc_type == COL_REL)
				{
				    rel =
					type_descriptors[i].grid_loc_def[j].
					col_rel;
				    if(nint(rel * nx) == x)
					{
					    match = TRUE;
					}
				}
			    if(match)
				{
				    priority =
					type_descriptors[i].grid_loc_def[j].
					priority;
				    column_type = &type_descriptors[i];
				}
			}
		}
	}
    return column_type;
}


static void
SetupTiming(IN t_options Options,
	    IN t_arch Arch,
	    IN boolean TimingEnabled,
	    IN enum e_operation Operation,
	    IN struct s_placer_opts PlacerOpts,
	    IN struct s_router_opts RouterOpts,
	    OUT t_timing_inf * Timing)
{

    /* Don't do anything if they don't want timing */
    if(FALSE == TimingEnabled)
	{
	    memset(Timing, 0, sizeof(t_timing_inf));
	    Timing->timing_analysis_enabled = FALSE;
	    return;
	}

    Timing->C_ipin_cblock = Arch.C_ipin_cblock;
    Timing->T_ipin_cblock = Arch.T_ipin_cblock;
    Timing->timing_analysis_enabled = TimingEnabled;
}


/* This loads up VPR's switch_inf data by combining the switches from 
 * the arch file with the special switches that VPR needs. */
static void
SetupSwitches(IN t_arch Arch,
	      INOUT struct s_det_routing_arch *RoutingArch,
	      IN struct s_switch_inf *ArchSwitches,
	      IN int NumArchSwitches)
{

    RoutingArch->num_switch = NumArchSwitches;

    /* Depends on RoutingArch->num_switch */
    RoutingArch->wire_to_ipin_switch = RoutingArch->num_switch;
    ++RoutingArch->num_switch;

    /* Depends on RoutingArch->num_switch */
    RoutingArch->delayless_switch = RoutingArch->num_switch;
    RoutingArch->global_route_switch = RoutingArch->delayless_switch;
    ++RoutingArch->num_switch;

    /* Alloc the list now that we know the final num_switch value */
    switch_inf =
	(struct s_switch_inf *)my_malloc(sizeof(struct s_switch_inf) *
					 RoutingArch->num_switch);

    /* Copy the switch data from architecture file */
    memcpy(switch_inf, ArchSwitches,
	   sizeof(struct s_switch_inf) * NumArchSwitches);

    /* Delayless switch for connecting sinks and sources with their pins. */
    switch_inf[RoutingArch->delayless_switch].buffered = TRUE;
    switch_inf[RoutingArch->delayless_switch].R = 0.;
    switch_inf[RoutingArch->delayless_switch].Cin = 0.;
    switch_inf[RoutingArch->delayless_switch].Cout = 0.;
    switch_inf[RoutingArch->delayless_switch].Tdel = 0.;

    /* The wire to ipin switch for all types. Curently all types
     * must share ipin switch. Some of the timing code would
     * need to be changed otherwise. */
    switch_inf[RoutingArch->wire_to_ipin_switch].buffered = TRUE;
    switch_inf[RoutingArch->wire_to_ipin_switch].R = 0.;
    switch_inf[RoutingArch->wire_to_ipin_switch].Cin = Arch.C_ipin_cblock;
    switch_inf[RoutingArch->wire_to_ipin_switch].Cout = 0.;
    switch_inf[RoutingArch->wire_to_ipin_switch].Tdel = Arch.T_ipin_cblock;
}


/* Sets up routing structures. Since checks are already done, this
 * just copies values across */
static void
SetupRoutingArch(IN t_arch Arch,
		 OUT struct s_det_routing_arch *RoutingArch)
{

    RoutingArch->switch_block_type = Arch.SBType;
    RoutingArch->R_minW_nmos = Arch.R_minW_nmos;
    RoutingArch->R_minW_pmos = Arch.R_minW_pmos;
    RoutingArch->Fs = Arch.Fs;
    RoutingArch->directionality = BI_DIRECTIONAL;
    if(Arch.Segments)
	RoutingArch->directionality = Arch.Segments[0].directionality;
}


static void
SetupRouterOpts(IN t_options Options,
		IN boolean TimingEnabled,
		OUT struct s_router_opts *RouterOpts)
{
	RouterOpts->astar_fac = 1.2;	/* DEFAULT */
	if(Options.Count[OT_ASTAR_FAC])
	{
	    RouterOpts->astar_fac = Options.astar_fac;
	}

    RouterOpts->bb_factor = 3;	/* DEFAULT */
	if(Options.Count[OT_FAST])
	{
		RouterOpts->bb_factor = 0;	/* DEFAULT */
	}
	if(Options.Count[OT_BB_FACTOR])
	{
	    RouterOpts->bb_factor = Options.bb_factor;
	}

    RouterOpts->criticality_exp = 1.0;	/* DEFAULT */
	if(Options.Count[OT_CRITICALITY_EXP])
	{
	    RouterOpts->criticality_exp = Options.criticality_exp;
	}

    RouterOpts->max_criticality = 0.99;	/* DEFAULT */
	if(Options.Count[OT_MAX_CRITICALITY])
	{
	    RouterOpts->max_criticality = Options.max_criticality;
	}

    RouterOpts->max_router_iterations = 50;	/* DEFAULT */
	if(Options.Count[OT_FAST])
	{
	    RouterOpts->max_router_iterations = 10;
	}
	if(Options.Count[OT_MAX_ROUTER_ITERATIONS])
	{
	    RouterOpts->max_router_iterations = Options.max_router_iterations;
	}

    RouterOpts->pres_fac_mult = 1.3;	/* DEFAULT */
	if(Options.Count[OT_PRES_FAC_MULT])
	{
	    RouterOpts->pres_fac_mult = Options.pres_fac_mult;
	}


    RouterOpts->route_type = DETAILED;	/* DEFAULT */
    if(Options.Count[OT_ROUTE_TYPE])
	{
	    RouterOpts->route_type = Options.RouteType;
	}

	RouterOpts->full_stats = FALSE; /* DEFAULT */
	if(Options.Count[OT_FULL_STATS])
	{
	    RouterOpts->full_stats = TRUE;
	}

	RouterOpts->verify_binary_search = FALSE; /* DEFAULT */
	if(Options.Count[OT_VERIFY_BINARY_SEARCH])
	{
	    RouterOpts->verify_binary_search = TRUE;
	}

    /* Depends on RouteOpts->route_type */
    RouterOpts->router_algorithm = DIRECTED_SEARCH;	/* DEFAULT */
    if(TimingEnabled)
	{
	    RouterOpts->router_algorithm = TIMING_DRIVEN;	/* DEFAULT */
	}
    if(GLOBAL == RouterOpts->route_type)
	{
	    RouterOpts->router_algorithm = DIRECTED_SEARCH;	/* DEFAULT */
	}
    if(Options.Count[OT_ROUTER_ALGORITHM])
	{
	    RouterOpts->router_algorithm = Options.RouterAlgorithm;
	}

    RouterOpts->fixed_channel_width = NO_FIXED_CHANNEL_WIDTH;	/* DEFAULT */
    if(Options.Count[OT_ROUTE_CHAN_WIDTH])
	{
	    RouterOpts->fixed_channel_width = Options.RouteChanWidth;
	}

    /* Depends on RouterOpts->router_algorithm */
    RouterOpts->initial_pres_fac = 0.5;	/* DEFAULT */
    if(DIRECTED_SEARCH == RouterOpts->router_algorithm || 
		Options.Count[OT_FAST])
	{
	    RouterOpts->initial_pres_fac = 10000.0;	/* DEFAULT */
	}
    if(Options.Count[OT_INITIAL_PRES_FAC])
	{
	    RouterOpts->initial_pres_fac = Options.initial_pres_fac;
	}

    /* Depends on RouterOpts->router_algorithm */
    RouterOpts->base_cost_type = DELAY_NORMALIZED;	/* DEFAULT */
    if(BREADTH_FIRST == RouterOpts->router_algorithm)
	{
	    RouterOpts->base_cost_type = DEMAND_ONLY;	/* DEFAULT */
	}
    if(DIRECTED_SEARCH == RouterOpts->router_algorithm)
	{
	    RouterOpts->base_cost_type = DEMAND_ONLY;	/* DEFAULT */
	}
	if(Options.Count[OT_BASE_COST_TYPE])
	{
	    RouterOpts->base_cost_type = Options.base_cost_type;
	}

    /* Depends on RouterOpts->router_algorithm */
    RouterOpts->first_iter_pres_fac = 0.5;	/* DEFAULT */
    if(BREADTH_FIRST == RouterOpts->router_algorithm)
	{
	    RouterOpts->first_iter_pres_fac = 0.0;	/* DEFAULT */
	}
    if(DIRECTED_SEARCH == RouterOpts->router_algorithm ||
		Options.Count[OT_FAST])
	{
	    RouterOpts->first_iter_pres_fac = 10000.0;	/* DEFAULT */
	}
	if(Options.Count[OT_FIRST_ITER_PRES_FAC])
	{
	    RouterOpts->first_iter_pres_fac = Options.first_iter_pres_fac;
	}

    /* Depends on RouterOpts->router_algorithm */
    RouterOpts->acc_fac = 1.0;
    if(BREADTH_FIRST == RouterOpts->router_algorithm)
	{
	    RouterOpts->acc_fac = 0.2;
	}
	if(Options.Count[OT_ACC_FAC])
	{
	    RouterOpts->acc_fac = Options.acc_fac;
	}

    /* Depends on RouterOpts->route_type */
    RouterOpts->bend_cost = 0.0;	/* DEFAULT */
    if(GLOBAL == RouterOpts->route_type)
	{
	    RouterOpts->bend_cost = 1.0;	/* DEFAULT */
	}
	if(Options.Count[OT_BEND_COST])
	{
	    RouterOpts->bend_cost = Options.bend_cost;
	}
}


static void
SetupAnnealSched(IN t_options Options,
		 OUT struct s_annealing_sched *AnnealSched)
{
    AnnealSched->alpha_t = 0.8;	/* DEFAULT */
    if(Options.Count[OT_ALPHA_T])
	{
	    AnnealSched->alpha_t = Options.PlaceAlphaT;
	}
    if(AnnealSched->alpha_t >= 1 || AnnealSched->alpha_t <= 0)
	{
	    printf(ERRTAG "alpha_t must be between 0 and 1 exclusive\n");
	    exit(1);
	}
    AnnealSched->exit_t = 0.01;	/* DEFAULT */
    if(Options.Count[OT_EXIT_T])
	{
	    AnnealSched->exit_t = Options.PlaceExitT;
	}
    if(AnnealSched->exit_t <= 0)
	{
	    printf(ERRTAG "exit_t must be greater than 0\n");
	    exit(1);
	}
    AnnealSched->init_t = 100.0;	/* DEFAULT */
    if(Options.Count[OT_INIT_T])
	{
	    AnnealSched->init_t = Options.PlaceInitT;
	}
    if(AnnealSched->init_t <= 0)
	{
	    printf(ERRTAG "init_t must be greater than 0\n");
	    exit(1);
	}
    if(AnnealSched->init_t < AnnealSched->exit_t)
	{
	    printf(ERRTAG "init_t must be greater or equal to than exit_t\n");
	    exit(1);
	}
    AnnealSched->inner_num = 10.0;	/* DEFAULT */
	if(Options.Count[OT_FAST]) {
		AnnealSched->inner_num = 1.0;	/* DEFAULT for fast*/
	}
    if(Options.Count[OT_INNER_NUM])
	{
	    AnnealSched->inner_num = Options.PlaceInnerNum;
	}
    if(AnnealSched->inner_num <= 0)
	{
	    printf(ERRTAG "init_t must be greater than 0\n");
	    exit(1);
	}
    AnnealSched->type = AUTO_SCHED;	/* DEFAULT */
    if((Options.Count[OT_ALPHA_T]) ||
       (Options.Count[OT_EXIT_T]) || (Options.Count[OT_INIT_T]))
	{
	    AnnealSched->type = USER_SCHED;
	}
}


/* Sets up the s_placer_opts structure based on users input. Error checking,
 * such as checking for conflicting params is assumed to be done beforehand */
static void
SetupPlacerOpts(IN t_options Options,
		IN boolean TimingEnabled,
		OUT struct s_placer_opts *PlacerOpts)
{
    PlacerOpts->block_dist = 1;	/* DEFAULT */
    if(Options.Count[OT_BLOCK_DIST])
	{
	    PlacerOpts->block_dist = Options.block_dist;
	}

    PlacerOpts->inner_loop_recompute_divider = 0;	/* DEFAULT */
    if(Options.Count[OT_INNER_LOOP_RECOMPUTE_DIVIDER])
	{
	    PlacerOpts->inner_loop_recompute_divider = Options.inner_loop_recompute_divider;
	}

    PlacerOpts->place_cost_exp = 1.0;	/* DEFAULT */
    if(Options.Count[OT_PLACE_COST_EXP])
	{
	    PlacerOpts->place_cost_exp = Options.place_cost_exp;
	}

    PlacerOpts->td_place_exp_first = 1;	/* DEFAULT */
    if(Options.Count[OT_TD_PLACE_EXP_FIRST])
	{
	    PlacerOpts->td_place_exp_first = Options.place_exp_first;
	}

    PlacerOpts->td_place_exp_last = 8;	/* DEFAULT */
    if(Options.Count[OT_TD_PLACE_EXP_LAST])
	{
	    PlacerOpts->td_place_exp_last = Options.place_exp_last;
	}

    PlacerOpts->place_cost_type = LINEAR_CONG;	/* DEFAULT */
    if(Options.Count[OT_PLACE_COST_TYPE])
	{
	    PlacerOpts->place_cost_type = Options.PlaceCostType;
	}

    /* Depends on PlacerOpts->place_cost_type */
    PlacerOpts->place_algorithm = BOUNDING_BOX_PLACE;	/* DEFAULT */
    if(TimingEnabled)
	{
	    PlacerOpts->place_algorithm = PATH_TIMING_DRIVEN_PLACE;	/* DEFAULT */
	}
    if(NONLINEAR_CONG == PlacerOpts->place_cost_type)
	{
	    PlacerOpts->place_algorithm = BOUNDING_BOX_PLACE;	/* DEFAULT */
	}
    if(Options.Count[OT_PLACE_ALGORITHM])
	{
	    PlacerOpts->place_algorithm = Options.PlaceAlgorithm;
	}

    PlacerOpts->num_regions = 4;	/* DEFAULT */
    if(Options.Count[OT_NUM_REGIONS])
	{
	    PlacerOpts->num_regions = Options.PlaceNonlinearRegions;
	}

    PlacerOpts->pad_loc_file = NULL;	/* DEFAULT */
    if(Options.Count[OT_FIX_PINS])
	{
	    if(Options.PinFile)
		{
		    PlacerOpts->pad_loc_file = my_strdup(Options.PinFile);
		}
	}

    PlacerOpts->pad_loc_type = FREE;	/* DEFAULT */
    if(Options.Count[OT_FIX_PINS])
	{
	    PlacerOpts->pad_loc_type = (Options.PinFile ? USER : RANDOM);
	}

    /* Depends on PlacerOpts->place_cost_type */
    PlacerOpts->place_chan_width = 100;	/* DEFAULT */
    if((NONLINEAR_CONG == PlacerOpts->place_cost_type) &&
       (Options.Count[OT_ROUTE_CHAN_WIDTH]))
	{
	    PlacerOpts->place_chan_width = Options.RouteChanWidth;
	}
    if(Options.Count[OT_PLACE_CHAN_WIDTH])
	{
	    PlacerOpts->place_chan_width = Options.PlaceChanWidth;
	}

    PlacerOpts->recompute_crit_iter = 1;	/* DEFAULT */
    if(Options.Count[OT_RECOMPUTE_CRIT_ITER])
	{
	    PlacerOpts->recompute_crit_iter = Options.RecomputeCritIter;
	}

    PlacerOpts->timing_tradeoff = 0.5;	/* DEFAULT */
    if(Options.Count[OT_TIMING_TRADEOFF])
	{
	    PlacerOpts->timing_tradeoff = Options.PlaceTimingTradeoff;
	}

    /* Depends on PlacerOpts->place_algorithm */
    PlacerOpts->enable_timing_computations = FALSE;	/* DEFAULT */
    if((PlacerOpts->place_algorithm == PATH_TIMING_DRIVEN_PLACE) ||
       (PlacerOpts->place_algorithm == NET_TIMING_DRIVEN_PLACE))
	{
	    PlacerOpts->enable_timing_computations = TRUE;	/* DEFAULT */
	}
    if(Options.Count[OT_ENABLE_TIMING_COMPUTATIONS])
	{
	    PlacerOpts->enable_timing_computations = Options.ShowPlaceTiming;
	}

    /* Depends on PlacerOpts->place_cost_type */
    PlacerOpts->place_freq = PLACE_ONCE;	/* DEFAULT */
    if(NONLINEAR_CONG == PlacerOpts->place_cost_type)
	{
	    PlacerOpts->place_freq = PLACE_ALWAYS;	/* DEFAULT */
	}
    if((Options.Count[OT_ROUTE_CHAN_WIDTH]) ||
       (Options.Count[OT_PLACE_CHAN_WIDTH]))
	{
	    PlacerOpts->place_freq = PLACE_ONCE;
	}
    if(Options.Count[OT_ROUTE_ONLY])
	{
	    PlacerOpts->place_freq = PLACE_NEVER;
	}
}


static void
SetupOperation(IN t_options Options,
	       OUT enum e_operation *Operation)
{
    *Operation = PLACE_AND_ROUTE;	/* DEFAULT */
    if(Options.Count[OT_ROUTE_ONLY])
	{
	    *Operation = ROUTE_ONLY;
	}
    if(Options.Count[OT_PLACE_ONLY])
	{
	    *Operation = PLACE_ONLY;
	}
    if(Options.Count[OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY])
	{
	    *Operation = TIMING_ANALYSIS_ONLY;
	}
}


/* Determines whether timing analysis should be on or off. 
   Unless otherwise specified, always default to timing.
*/
boolean
IsTimingEnabled(IN t_options Options)
{
    /* First priority to the '--timing_analysis' flag */
    if(Options.Count[OT_TIMING_ANALYSIS])
	{
	    return Options.TimingAnalysis;
	}
    return TRUE;
}

/* If a block for a given type does not have subblocks, this creates a subblock for it.
   This is used to setup I/Os because I/Os have no subblocks in the netlist */
static void
load_subblock_info_to_type(INOUT t_subblock_data * subblocks,
			   INOUT t_type_ptr type)
{
    int iblk, i;
    int *num_subblocks_per_block;
    t_subblock **subblock_inf;

    num_subblocks_per_block = subblocks->num_subblocks_per_block;
    subblock_inf = subblocks->subblock_inf;

    /* This is also a hack, IO's have subblocks prespecified */
    if(type != IO_TYPE)
	{
	    type_descriptors[type->index].max_subblock_inputs =
		type->num_receivers;
	    type_descriptors[type->index].max_subblock_outputs =
		type->num_drivers;
	    type_descriptors[type->index].max_subblocks = 1;
	}

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    if(block[iblk].type == type)
		{
		    subblock_inf[iblk] =
			(t_subblock *) my_malloc(sizeof(t_subblock));
		    num_subblocks_per_block[iblk] = 1;
		    subblock_inf[iblk][0].name = block[iblk].name;
		    subblock_inf[iblk][0].inputs =
			(int *)my_malloc(type->max_subblock_inputs *
					 sizeof(int));
		    subblock_inf[iblk][0].outputs =
			(int *)my_malloc(type->max_subblock_outputs *
					 sizeof(int));
		    for(i = 0; i < type->num_pins; i++)
			{
			    if(i < type->max_subblock_inputs)
				{
				    subblock_inf[iblk][0].inputs[i] =
					(block[iblk].nets[i] ==
					 OPEN) ? OPEN : i;
				}
			    else if(i <
				    type->max_subblock_inputs +
				    type->max_subblock_outputs)
				{
				    subblock_inf[iblk][0].outputs[i -
								  type->
								  max_subblock_inputs]
					=
					(block[iblk].nets[i] ==
					 OPEN) ? OPEN : i;
				}
			    else
				{
				    subblock_inf[iblk][0].clock =
					(block[iblk].nets[i] ==
					 OPEN) ? OPEN : i;
				}
			}
		}
	}
}
