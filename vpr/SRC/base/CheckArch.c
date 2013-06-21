#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "OptionTokens.h"
#include "ReadOptions.h"
#include "read_xml_arch_file.h"
#include "SetupVPR.h"

/******** Function Prototypes ********/
static void CheckSwitches(INP t_arch Arch, INP boolean TimingEnabled);

static void CheckSegments(INP t_arch Arch);

/******** Function Implementations ********/

void CheckArch(INP t_arch Arch, INP boolean TimingEnabled) {
	CheckSwitches(Arch, TimingEnabled);
	CheckSegments(Arch);
}

static void CheckSwitches(INP t_arch Arch, INP boolean TimingEnabled) {
	struct s_switch_inf *CurSwitch;
	int i;

	/* Check transistors in switches won't be less than minimum size */
	CurSwitch = Arch.Switches;
	for (i = 0; i < Arch.num_switches; i++) {
		/* This assumes all segments have the same directionality */
		if (CurSwitch->buffered
				&& Arch.Segments[0].directionality == BI_DIRECTIONAL) {
			/* Largest resistance tri-state buffer would have a minimum 
			 * width transistor in the buffer pull-down and a min-width 
			 * pass transistoron the output.  
			 * Hence largest R = 2 * largest_transistor_R. */
			if (CurSwitch->R > 2 * Arch.R_minW_nmos) {
				vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0, 
						"Switch %s R value (%g) is greater than 2 * R_minW_nmos (%g).\n"
						"Refer to switchlist section of '%s'\n", 
						CurSwitch->name, CurSwitch->R, (2 * Arch.R_minW_nmos));
			}
		} else { /* Pass transistor switch */
			if (CurSwitch->R > Arch.R_minW_nmos) {
				vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0, 
						"Switch %s R value (%g) is greater than R_minW_nmos (%g).\n"
						"Refer to switchlist section of '%s'\n", 
						CurSwitch->name, CurSwitch->R, Arch.R_minW_nmos, get_arch_file_name());
			}
		}
	}
}

static void CheckSegments(INP t_arch Arch) {
	t_segment_inf *CurSeg;
	int i;

	CurSeg = Arch.Segments;
	for (i = 0; i < Arch.num_segments; i++) {
		if (CurSeg[i].directionality == UNI_DIRECTIONAL
				&& CurSeg[i].longline == TRUE) {
			vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0, 
					"Long lines not supported for unidirectional architectures.\n"
					"Refer to segmentlist of '%s'\n",get_arch_file_name());
		}
	}
}
