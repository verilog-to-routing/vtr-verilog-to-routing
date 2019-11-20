#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"

/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */


/******************** Subroutine definitions ********************************/


boolean is_opin (int ipin) {

/* Returns TRUE if this clb pin is an output, FALSE otherwise. */

 int iclass;

 iclass = clb_pin_class[ipin];

 if (class_inf[iclass].type == DRIVER)
    return (TRUE);
 else
    return (FALSE);
}


void load_one_clb_fanout_count (int subblock_lut_size, t_subblock
         *subblock_inf, int num_subblocks, int *num_uses_of_clb_ipin,
         int *num_uses_of_sblk_opin, int iblk) {

 
/* Loads the fanout counts for one block (iblk).  */
 
 int isub, ipin, conn_pin;

 for (isub=0;isub<num_subblocks;isub++) {
 
  /* Is the subblock output connected to a CLB opin that actually goes *
   * somewhere?  Necessary to check that the CLB opin connects to      *
   * something because some logic blocks result in netlists where      *
   * subblock outputs being automatically hooked to a CLB opin under   *
   * all conditions.                                                   */

    conn_pin = subblock_inf[isub].output;
    if (conn_pin != OPEN) {
       if (block[iblk].nets[conn_pin] != OPEN) {  /* CLB output is used */
          num_uses_of_sblk_opin[isub]++;
       }
    }
 
    for (ipin=0;ipin<subblock_lut_size;ipin++) {
       conn_pin = subblock_inf[isub].inputs[ipin];
 
       if (conn_pin != OPEN) {
          if (conn_pin < pins_per_clb) {   /* Driven by CLB ipin */
             num_uses_of_clb_ipin[conn_pin]++;
          }
          else {   /* Driven by sblk output in same clb */
             num_uses_of_sblk_opin[conn_pin - pins_per_clb]++;
          }
       }   
    }  /* End for each sblk ipin */

    conn_pin = subblock_inf[isub].clock;  /* Now do clock pin */

    if (conn_pin != OPEN) {
       if (conn_pin < pins_per_clb) {   /* Driven by CLB ipin */
          num_uses_of_clb_ipin[conn_pin]++;
       } 
       else {   /* Driven by sblk output in same clb */
          num_uses_of_sblk_opin[conn_pin - pins_per_clb]++;
       } 
    }   

 }  /* End for each subblock */
}
