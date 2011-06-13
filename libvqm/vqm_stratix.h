/* VQM Stratix Specific WYSIWYG definitions file
 *
 * Author:	Tomasz Czajkowski
 * Date:	September 18, 2004
 * NOTES/REVISIONS:
 * January 6, 2006 - Adding Stratix II support.
 */

#if !defined (__STRATIX_WYSIWYG_DEFINITIONS__)
#define __STRATIX_WYSIWYG_DEFINITIONS__

typedef enum {	/* STRATIX */
					STRATIX_IO_CELL = 0,
					STRATIX_LCELL,
					STRATIX_MAC_MULT,
					STRATIX_MAC_OUT,
					STRATIX_RAM_BLOCK,
				/* Stratix II */
					STRATIX_II_IO_CELL,
					STRATIX_II_LCELL_COMB,
					STRATIX_II_LCELL_FF,
					STRATIX_II_RAM_BLOCK,
					STRATIX_II_MAC_MULT, 
					STRATIX_II_MAC_OUT,
					STRATIX_II_DLL,
					STRATIX_II_PLL,
					STRATIX_II_CLOCK_BUF
			} t_stratix_cells;

#endif