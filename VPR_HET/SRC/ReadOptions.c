#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "OptionTokens.h"
#include "ReadOptions.h"

/******** Function prototypes ********/

static const char *const *ReadBaseToken(IN const char *const *Args,
					OUT enum e_OptionBaseToken *Token);
static void Error(IN const char *Token);
static void ErrorOption(IN const char *Option);
static const char *const *ProcessOption(IN const char *const *Args,
					INOUT t_options * Options);
static const char *const *ReadFloat(IN const char *const *Args,
				    OUT float *Val);
static const char *const *ReadInt(IN const char *const *Args,
				  OUT int *Val);
static const char *const *ReadOnOff(IN const char *const *Args,
				    OUT boolean * Val);
static const char *const *ReadFixPins(IN const char *const *Args,
				      OUT char **PinFile);
static const char *const *ReadPlaceAlgorithm(IN const char *const *Args,
					     OUT enum e_place_algorithm
					     *Algo);
static const char *const *ReadPlaceCostType(IN const char *const *Args,
					    OUT enum place_c_types *Type);
static const char *const *ReadRouterAlgorithm(IN const char *const *Args,
					      OUT enum e_router_algorithm
					      *Algo);
static const char *const *ReadBaseCostType(IN const char *const *Args,
					OUT enum e_base_cost_type *BaseCostType);
static const char *const *ReadRouteType(IN const char *const *Args,
					OUT enum e_route_type *Type);
static const char *const *ReadString(IN const char *const *Args,
				     OUT char **Val);


/******** Subroutine implementations ********/

void
ReadOptions(IN int argc,
	    IN char **argv,
	    OUT t_options * Options)
{
    char **Args, **head;

    /* Clear values and pointers to zero */
    memset(Options, 0, sizeof(t_options));

    /* Alloc a new pointer list for args with a NULL at end.
     * This makes parsing the same as for archfile for consistency.
     * Skips the first arg as it is the program image path */
    --argc;
    ++argv;
    head = Args = (char **)my_malloc(sizeof(char *) * (argc + 1));
    memcpy(Args, argv, (sizeof(char *) * argc));
    Args[argc] = NULL;

    /* Go through the command line args. If they have hyphens they are 
     * options. Otherwise assume they are part of the four mandatory
     * arguments */
    while(*Args)
	{
	    if(strncmp("--", *Args, 2) == 0)
		{
		    *Args += 2;	/* Skip the prefix */
		    Args =
			(char **)ProcessOption((const char *const *)Args,
					       Options);
		}
	    else if(strncmp("-", *Args, 1) == 0)
		{
		    *Args += 1;	/* Skip the prefix */
		    Args =
			(char **)ProcessOption((const char *const *)Args,
					       Options);
		}
	    else if(NULL == Options->NetFile)
		{
		    Options->NetFile = my_strdup(*Args);
		    ++Args;
		}
	    else if(NULL == Options->ArchFile)
		{
		    Options->ArchFile = my_strdup(*Args);
		    ++Args;
		}
	    else if(NULL == Options->PlaceFile)
		{
		    Options->PlaceFile = my_strdup(*Args);
		    ++Args;
		}
	    else if(NULL == Options->RouteFile)
		{
		    Options->RouteFile = my_strdup(*Args);
		    ++Args;
		}
	    else
		{
		    /* Not an option and arch and net already specified so fail */
		    Error(*Args);
		}
	}
	free(head);
}


static const char *const *
ProcessOption(IN const char *const *Args,
	      INOUT t_options * Options)
{
    enum e_OptionBaseToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadBaseToken(Args, &Token);

    if(Token < OT_BASE_UNKNOWN)
	{
	    ++Options->Count[Token];
	}

    switch (Token)
	{
	/* General Options */
    case OT_NODISP:
		return Args;
	case OT_AUTO:
	    return ReadInt(Args, &Options->GraphPause);
	case OT_ROUTE_ONLY:
	case OT_PLACE_ONLY:
	    return Args;
	case OT_TIMING_ANALYZE_ONLY_WITH_NET_DELAY: 
		return ReadFloat(Args, &Options->constant_net_delay);
	case OT_FAST:	
	case OT_FULL_STATS:
	    return Args;
	case OT_TIMING_ANALYSIS:
	    return ReadOnOff(Args, &Options->TimingAnalysis);
	case OT_OUTFILE_PREFIX:
	    return ReadString(Args, &Options->OutFilePrefix);

	/* Placer Options */
	case OT_PLACE_ALGORITHM:
	    return ReadPlaceAlgorithm(Args, &Options->PlaceAlgorithm);
	case OT_INIT_T:
	    return ReadFloat(Args, &Options->PlaceInitT);
	case OT_EXIT_T:
	    return ReadFloat(Args, &Options->PlaceExitT);
	case OT_ALPHA_T:
	    return ReadFloat(Args, &Options->PlaceAlphaT);
	case OT_INNER_NUM:
	    return ReadFloat(Args, &Options->PlaceInnerNum);
	case OT_SEED:
	    return ReadInt(Args, &Options->Seed);
	case OT_PLACE_COST_EXP:
		return ReadFloat(Args, &Options->place_cost_exp);
	case OT_PLACE_COST_TYPE:
	    return ReadPlaceCostType(Args, &Options->PlaceCostType);
	case OT_PLACE_CHAN_WIDTH:
	    return ReadInt(Args, &Options->PlaceChanWidth);
	case OT_NUM_REGIONS:
	    return ReadInt(Args, &Options->PlaceNonlinearRegions);
	case OT_FIX_PINS:
	    return ReadFixPins(Args, &Options->PinFile);
	case OT_ENABLE_TIMING_COMPUTATIONS:
		return ReadOnOff(Args, &Options->ShowPlaceTiming);
	case OT_BLOCK_DIST:
		return ReadInt(Args, &Options->block_dist);

	/* Placement Options Valid Only for Timing-Driven Placement */
	case OT_TIMING_TRADEOFF:
	    return ReadFloat(Args, &Options->PlaceTimingTradeoff);
	case OT_RECOMPUTE_CRIT_ITER:
	    return ReadInt(Args, &Options->RecomputeCritIter);
	case OT_INNER_LOOP_RECOMPUTE_DIVIDER:
		return ReadInt(Args, &Options->inner_loop_recompute_divider);
	case OT_TD_PLACE_EXP_FIRST:
		return ReadFloat(Args, &Options->place_exp_first);
	case OT_TD_PLACE_EXP_LAST:
		return ReadFloat(Args, &Options->place_exp_last);

	/* Router Options */
	case OT_MAX_ROUTER_ITERATIONS:
		return ReadInt(Args, &Options->max_router_iterations);
	case OT_BB_FACTOR:
		return ReadInt(Args, &Options->bb_factor);
	case OT_INITIAL_PRES_FAC:
		return ReadFloat(Args, &Options->initial_pres_fac);
	case OT_PRES_FAC_MULT:
		return ReadFloat(Args, &Options->pres_fac_mult);
	case OT_ACC_FAC:
		return ReadFloat(Args, &Options->acc_fac);
	case OT_FIRST_ITER_PRES_FAC:
		return ReadFloat(Args, &Options->first_iter_pres_fac);
	case OT_BEND_COST:
		return ReadFloat(Args, &Options->bend_cost);
	case OT_ROUTE_TYPE:
	    return ReadRouteType(Args, &Options->RouteType);
	case OT_VERIFY_BINARY_SEARCH:
		return Args;
	case OT_ROUTE_CHAN_WIDTH:
	    return ReadInt(Args, &Options->RouteChanWidth);
	case OT_ROUTER_ALGORITHM:
	    return ReadRouterAlgorithm(Args, &Options->RouterAlgorithm);
	case OT_BASE_COST_TYPE:
		return ReadBaseCostType(Args, &Options->base_cost_type);

	/* Routing options valid only for timing-driven routing */
	case OT_ASTAR_FAC:
		return ReadFloat(Args, &Options->astar_fac);
	case OT_MAX_CRITICALITY:
		return ReadFloat(Args, &Options->max_criticality);
	case OT_CRITICALITY_EXP:
		return ReadFloat(Args, &Options->criticality_exp);
	default:
	    ErrorOption(*PrevArgs);
	}
    return NULL;
}


static const char *const *
ReadBaseToken(IN const char *const *Args,
	      OUT enum e_OptionBaseToken *Token)
{
    const struct s_TokenPair *Cur;

    /* Empty string is end of tokens marker */
    if(NULL == *Args)
	Error(*Args);

    /* Linear search for the pair */
    Cur = OptionBaseTokenList;
    while(Cur->Str)
	{
	    if(strcmp(*Args, Cur->Str) == 0)
		{
		    *Token = Cur->Enum;
		    return ++Args;
		}
	    ++Cur;
	}

    *Token = OT_BASE_UNKNOWN;
    return ++Args;
}


static const char *const *
ReadToken(IN const char *const *Args,
	  OUT enum e_OptionArgToken *Token)
{
    const struct s_TokenPair *Cur;

    /* Empty string is end of tokens marker */
    if(NULL == *Args)
	Error(*Args);

    /* Linear search for the pair */
    Cur = OptionArgTokenList;
    while(Cur->Str)
	{
	    if(strcmp(*Args, Cur->Str) == 0)
		{
		    *Token = Cur->Enum;
		    return ++Args;
		}
	    ++Cur;
	}

    *Token = OT_ARG_UNKNOWN;
    return ++Args;
}


/* Called for parse errors. Spits out a message and then exits program. */
static void
Error(IN const char *Token)
{
    if(Token)
	{
	    printf(ERRTAG "Unexpected token '%s' on command line\n", Token);
	}
    else
	{
	    printf(ERRTAG "Missing token at end of command line\n");
	}
    exit(1);
}


static void
ErrorOption(IN const char *Option)
{
    printf(ERRTAG "Unexpected option '%s' on command line\n", Option);
    exit(1);
}


static const char *const *
ReadRouterAlgorithm(IN const char *const *Args,
		    OUT enum e_router_algorithm *Algo)
{
    enum e_OptionArgToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadToken(Args, &Token);
    switch (Token)
	{
	case OT_BREADTH_FIRST:
	    *Algo = BREADTH_FIRST;
	    break;
	case OT_DIRECTED_SEARCH:
	    *Algo = DIRECTED_SEARCH;
	    break;
	case OT_TIMING_DRIVEN:
	    *Algo = TIMING_DRIVEN;
	    break;
	default:
	    Error(*PrevArgs);
	}

    return Args;
}

static const char *const *
ReadBaseCostType(IN const char *const *Args,
		    OUT enum e_base_cost_type *BaseCostType)
{
    enum e_OptionArgToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadToken(Args, &Token);
    switch (Token)
	{
	case OT_INTRINSIC_DELAY:
	    *BaseCostType = INTRINSIC_DELAY;
	    break;
	case OT_DELAY_NORMALIZED:
	    *BaseCostType = DELAY_NORMALIZED;
	    break;
	case OT_DEMAND_ONLY:
	    *BaseCostType = DEMAND_ONLY;
	    break;
	default:
	    Error(*PrevArgs);
	}

    return Args;
}


static const char *const *
ReadRouteType(IN const char *const *Args,
	      OUT enum e_route_type *Type)
{
    enum e_OptionArgToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadToken(Args, &Token);
    switch (Token)
	{
	case OT_GLOBAL:
	    *Type = GLOBAL;
	    break;
	case OT_DETAILED:
	    *Type = DETAILED;
	    break;
	default:
	    Error(*PrevArgs);
	}

    return Args;
}


static const char *const *
ReadPlaceCostType(IN const char *const *Args,
		  OUT enum place_c_types *Type)
{
    enum e_OptionArgToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadToken(Args, &Token);
    switch (Token)
	{
	case OT_LINEAR:
	    *Type = LINEAR_CONG;
	    break;
	case OT_NONLINEAR:
	    *Type = NONLINEAR_CONG;
	    break;
	default:
	    Error(*PrevArgs);
	}

    return Args;
}


static const char *const *
ReadPlaceAlgorithm(IN const char *const *Args,
		   OUT enum e_place_algorithm *Algo)
{
    enum e_OptionArgToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadToken(Args, &Token);
    switch (Token)
	{
	case OT_BOUNDING_BOX:
	    *Algo = BOUNDING_BOX_PLACE;
	    break;
	case OT_NET_TIMING_DRIVEN:
	    *Algo = NET_TIMING_DRIVEN_PLACE;
	    break;
	case OT_PATH_TIMING_DRIVEN:
	    *Algo = PATH_TIMING_DRIVEN_PLACE;
	    break;
	default:
	    Error(*PrevArgs);
	}

    return Args;
}

static const char *const *
ReadFixPins(IN const char *const *Args,
	    OUT char **PinFile)
{
    enum e_OptionArgToken Token;
    int Len;
	const char *const *PrevArgs = Args;

    Args = ReadToken(Args, &Token);
    if(OT_RANDOM != Token)
	{
	    Len = 1 + strlen(*PrevArgs);
	    *PinFile = (char *)my_malloc(Len * sizeof(char));
	    memcpy(*PinFile, *PrevArgs, Len);
	}
    return Args;
}


static const char *const *
ReadOnOff(IN const char *const *Args,
	  OUT boolean * Val)
{
    enum e_OptionArgToken Token;
    const char *const *PrevArgs;

    PrevArgs = Args;
    Args = ReadToken(Args, &Token);
    switch (Token)
	{
	case OT_ON:
	    *Val = TRUE;
	    break;
	case OT_OFF:
	    *Val = FALSE;
	    break;
	default:
	    Error(*PrevArgs);
	}
    return Args;
}


static const char *const *
ReadInt(IN const char *const *Args,
	OUT int *Val)
{
    if(NULL == *Args)
	Error(*Args);
    if((**Args > '9') || (**Args < '0'))
	Error(*Args);

    *Val = atoi(*Args);

    return ++Args;
}


static const char *const *
ReadFloat(IN const char *const *Args,
	  OUT float *Val)
{
    if(NULL == *Args)
	{
	    Error(*Args);
	}

    if((**Args != '-') &&
       (**Args != '.') && ((**Args > '9') || (**Args < '0')))
	{
	    Error(*Args);
	}

    *Val = atof(*Args);

    return ++Args;
}


static const char *const *
ReadString(IN const char *const *Args,
	   OUT char **Val)
{
    if(NULL == *Args)
	{
	    Error(*Args);
	}

    *Val = my_strdup(*Args);

    return ++Args;
}
