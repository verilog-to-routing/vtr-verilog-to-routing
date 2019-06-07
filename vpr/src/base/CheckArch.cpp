#include <string.h>

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "echo_files.h"
#include "read_xml_arch_file.h"
#include "CheckArch.h"

/******** Function Prototypes ********/
static void CheckSwitches(const t_arch& Arch);

static void CheckSegments(const t_arch& Arch);

/******** Function Implementations ********/

void CheckArch(const t_arch& Arch) {
    CheckSwitches(Arch);
    CheckSegments(Arch);
}

static void CheckSwitches(const t_arch& Arch) {
    t_arch_switch_inf* CurSwitch;
    int i;
    int ipin_cblock_switch_index = UNDEFINED;

    /* Check transistors in switches won't be less than minimum size */
    CurSwitch = Arch.Switches;
    for (i = 0; i < Arch.num_switches; i++) {
        /* This assumes all segments have the same directionality */
        if (CurSwitch->buffered()
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

        /* find the ipin cblock switch index, if it exists */
        if (Arch.Switches[i].name == Arch.ipin_cblock_switch_name) {
            ipin_cblock_switch_index = i;
        }
    }

    /* Check that the ipin cblock switch doesn't specify multiple (#inputs,delay) pairs. This
     * is not currently allowed because a number of functions in VPR rely on being able
     * to access the ipin cblock switch's Tdel using the wire_to_ipin_switch variable.
     * If we allow multiple fanins in the future, we would have to change the wire_to_ipin_switch
     * index to point to a switch with a routing resource switch with a representative Tdel value.
     * See rr_graph.c:alloc_and_load_rr_switch_inf for more info */
    if (ipin_cblock_switch_index != UNDEFINED) {
        if (!Arch.Switches[ipin_cblock_switch_index].fixed_Tdel()) {
            vpr_throw(VPR_ERROR_ARCH, __FILE__, __LINE__,
                      "Not currently allowing an ipin cblock switch to have fanin dependent values");
        }
    }
}

static void CheckSegments(const t_arch& Arch) {
    auto& CurSeg = Arch.Segments;
    for (size_t i = 0; i < (Arch.Segments).size(); i++) {
        if (CurSeg[i].directionality == UNI_DIRECTIONAL
            && CurSeg[i].longline == true) {
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0,
                      "Long lines not supported for unidirectional architectures.\n"
                      "Refer to segmentlist of '%s'\n",
                      get_arch_file_name());
        }
    }
}
