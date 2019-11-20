#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "xml_arch.h"
#include "SetupVPR.h"

/******** Function Prototypes ********/
static void CheckSwitches(IN t_arch Arch,
			  IN boolean TimingEnabled);

static void CheckSegments(IN t_arch Arch);

/******** Function Implementations ********/

void
CheckArch(IN t_arch Arch,
	  IN boolean TimingEnabled)
{
	CheckSwitches(Arch, TimingEnabled);
	CheckSegments(Arch);
}

static void
CheckSwitches(IN t_arch Arch,
	      IN boolean TimingEnabled)
{
    struct s_switch_inf *CurSwitch;
    int i;

    /* Check transistors in switches won't be less than minimum size */
    CurSwitch = Arch.Switches;
    for(i = 0; i < Arch.num_switches; i++)
	{
		/* This assumes all segments have the same directionality */
		if(CurSwitch->buffered && Arch.Segments[0].directionality == BI_DIRECTIONAL)
		{
		    /* Largest resistance tri-state buffer would have a minimum 
		     * width transistor in the buffer pull-down and a min-width 
		     * pass transistoron the output.  
		     * Hence largest R = 2 * largest_transistor_R. */
		    if(CurSwitch->R > 2 * Arch.R_minW_nmos)
			{
			    printf(ERRTAG
				   "Switch %s R value (%g) is greater than "
				   "2 * R_minW_nmos (%g).\n", CurSwitch->name,
				   CurSwitch->R, (2 * Arch.R_minW_nmos));
			    exit(1);
			}
		}
	    else
		{		/* Pass transistor switch */
		    if(CurSwitch->R > Arch.R_minW_nmos)
			{
			    printf(ERRTAG
				   "Switch %s R value (%g) is greater than "
				   "R_minW_nmos (%g).\n", CurSwitch->name,
				   CurSwitch->R, Arch.R_minW_nmos);
			    exit(1);
			}
		}
	}
}

static void CheckSegments(IN t_arch Arch) {
	t_segment_inf *CurSeg;
    int i;

    CurSeg = Arch.Segments;
    for(i = 0; i < Arch.num_segments; i++)
	{
		if(CurSeg[i].directionality == UNI_DIRECTIONAL && CurSeg[i].longline == TRUE) {
			printf("Long lines not supported for unidirectional architectures\n");
			exit(1);
		}
	}
}
