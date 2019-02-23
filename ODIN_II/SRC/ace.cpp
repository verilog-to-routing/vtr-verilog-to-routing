/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include <stdlib.h>
#include <stdio.h>

#include "misc/util/abc_namespaces.h"
#include "misc/st/st.h"
#include "bdd/cudd/cuddInt.h"
#include "bdd/cudd/cudd.h"

#include "odin_error.h"
#include "odin_util.h"
#include "ace.h"
#include "vtr_memory.h"

#define ACE_P0TO1(P1,PS)		((P1)==0.0)?0.0:(((P1)==1.0)?1.0:0.5*PS/(1.0-(P1)))
#define ACE_P1TO0(P1,PS)		((P1)==0.0)?1.0:(((P1)==0.0)?0.0:0.5*PS/(P1))

#define MAX(a,b) 			(a > b ? a : b)

/*   Cube routines from Espresso-mv   */
#define TWO     3
#define DASH    3
#define ONE     2
#define ZERO    1

#define BPI     32
#define LOGBPI  5

#define LOOPINIT(size)		((size <= BPI) ? 1 : WHICH_WORD((size)-1))

#define WHICH_WORD(element)     (((element) >> LOGBPI) + 1)
#define WHICH_BIT(element)      ((element) & (BPI-1))

#define GETINPUT(c, pos)	((c[WHICH_WORD(2*pos)] >> WHICH_BIT(2*pos)) & 3)

#define node_get_literal(c_, j_)   ((c_) ? GETINPUT((c_), (j_)) :  node_error(2))

typedef unsigned int *pset;

#define ALLOC(type, num)	((type *) malloc(sizeof(type) * (num)))

#define set_remove(set, e)      (set[WHICH_WORD(e)] &= ~ (shift_left_value_with_overflow_check(0x1, WHICH_BIT(e))))
#define set_insert(set, e)      (set[WHICH_WORD(e)] |= (shift_left_value_with_overflow_check(0x1, WHICH_BIT(e))))
#define set_new(size)		set_clear(ALLOC(unsigned int, set_size(size)), size)
#define set_size(size)          ((size) <= BPI ? 2 : (WHICH_WORD((size)-1) + 1))

typedef struct {
	pset cube;
	int num_literals;
	double static_prob;
} ace_cube_t;


/******** Function prototypes ********/

ace_obj_info_t* alloc_and_init_ace_info_object ();
int calc_node_depth(nnode_t *node );
void compute_switching_activities ( netlist_t *net );
void prob_epsilon_fix(double * d);
void output_ace_info (netlist_t *net, FILE *act_out );
void output_ace_info_node ( char *name, ace_obj_info_t *info, FILE *act_out );
pset set_clear(pset r, int size);
ace_cube_t * ace_cube_dup(ace_cube_t * cube);
ace_cube_t * ace_cube_new_dc(int num_literals);
DdNode * build_bdd_for_node ( DdManager * dd, nnode_t *node );
void ace_bdd_count_paths(DdManager * mgr, DdNode * bdd, int * num_one_paths, int * num_zero_paths);
double calc_cube_switch_prob_recur(DdManager * mgr, DdNode * bdd, ace_cube_t * cube, nnode_t *node , st__table *visited, int phase );
double calc_cube_switch_prob(DdManager * mgr, DdNode * bdd, ace_cube_t * cube, nnode_t *node, int phase );
double calc_switch_prob_recur(DdManager * mgr, DdNode * bdd_next, DdNode * bdd, ace_cube_t * cube, nnode_t *node, double P1, int phase );
int node_error(int code);


/*---------------------------------------------------------------------------
 * (function: alloc_and_init_ace_structs )
 *
 * Allocates and initializes Ace Objects for a given NetList
 *
 *---------------------------------------------------------------------------*/
void alloc_and_init_ace_structs(netlist_t *net) {

    int x,y;

    /*
    printf ( "alloc_and_init_activity_info\n");
    printf ( "----------------------------\n");
    printf ("PIs in network: %ld\n",  net->num_top_input_nodes);
    printf ("POs in network: %ld\n",  net->num_top_output_nodes);

    printf ("Nodes in network: %ld\n",  net->num_internal_nodes);
    printf ("FF Nodes in network: %ld\n",  net->num_ff_nodes);
    printf ("Forward Level Nodes in network: %ld\n",  net->num_forward_levels);
    printf ("Backward Level Nodes in network: %ld\n",  net->num_backward_levels);
    */

    // GND Node
    net->gnd_node->output_pins[0]->ace_info = alloc_and_init_ace_info_object();

    // VCC Node
    net->vcc_node->output_pins[0]->ace_info = alloc_and_init_ace_info_object();

    // unconn Node
    net->pad_node->output_pins[0]->ace_info = alloc_and_init_ace_info_object();

    // Input Nodes
    for ( x = 0; x < net->num_top_input_nodes; x++ ) {
        for ( y = 0; y < net->top_input_nodes[x]->num_output_pins; y++ ) {
            net->top_input_nodes[x]->output_pins[y]->ace_info = alloc_and_init_ace_info_object();
        }
    }

    // FF Nodes
    for ( x = 0; x < net->num_ff_nodes; x++ ) {
        for ( y = 0; y < net->ff_nodes[x]->num_output_pins; y++ ) {
            net->ff_nodes[x]->output_pins[y]->ace_info = alloc_and_init_ace_info_object();
        }
    }

    // Internal Nodes
    for ( x = 0; x < net->num_internal_nodes; x++ ) {
        for ( y = 0; y < net->internal_nodes[x]->num_output_pins; y++ ) {
            net->internal_nodes[x]->output_pins[y]->ace_info = alloc_and_init_ace_info_object();
        }
    }

    // Calculate depth of nodes. Must have ace_info object allocated first
    for ( x = 0; x < net->num_internal_nodes; x++ ) {
        for ( y = 0; y < net->internal_nodes[x]->num_output_pins; y++ ) {
            net->internal_nodes[x]->output_pins[y]->ace_info->depth = calc_node_depth ( net->internal_nodes[x] );
        }
    }

}


/*---------------------------------------------------------------------------
 * (function: alloc_and_init_ace_info_object )
 *
 * Allocates, initializes and returns an Ace Object
 *
 *---------------------------------------------------------------------------*/
ace_obj_info_t* alloc_and_init_ace_info_object() {

    ace_obj_info_t *new_node;

    new_node = (ace_obj_info_t *)my_malloc_struct(sizeof(ace_obj_info_t));

    new_node->value = -1;
    new_node->num_toggles = 0;
    new_node->num_ones = 0;
    new_node->depth = 0;

    return new_node;
}


/*---------------------------------------------------------------------------
 * (function: calc_node_depth )
 *
 * Determine the depth of a given node within its netlist
 *
 *---------------------------------------------------------------------------*/
int calc_node_depth(nnode_t *node ) {

    int x, depth = 0;
    ace_obj_info_t * fanin_info;

    for ( x = 0;  x < node->num_input_pins; x++ ) {
        fanin_info = node->input_pins[x]->net->driver_pin->ace_info;
        depth = MAX ( depth, fanin_info->depth );
    }
    return depth + 1;
}


/*---------------------------------------------------------------------------------------------
 * (function: calculate_activity )
 *
 * Calculates activity stats for a given netlist
 *
 *-------------------------------------------------------------------------------------------*/
void calculate_activity ( netlist_t *net, int max_cycles, FILE *act_out ) {

    int x,y;
    ace_obj_info_t *info;

    /*
    printf ( "Calculating Activity Stats\n");
    printf ( "--------------------------\n");
    */

    // Process Top Input Nodes
    for ( x = 0; x < net->num_top_input_nodes; x++ ) {
        info = net->top_input_nodes[x]->output_pins[0]->ace_info;

        // Odin processes each vector 2 times, need to adjust count.
        info->num_ones = info->num_ones / 2;

        // Static Probability = NUMBER of ONES / Cycles
        info->static_prob = (double ) info->num_ones / (double) max_cycles;

        // Clock should toggle once per cycle
        if ( net->top_input_nodes[x]->type == CLOCK_NODE ) {
            info->num_toggles = max_cycles;
        }

        // Switching Probability = NUMBER of TOGGLES / Cycles
        info->switch_prob = info->num_toggles / (double) max_cycles;

        // Switch Activity = same as Switching Probability for top inputs.
        info->switch_act  = info->switch_prob;
    }

    // Process FF Nodes
    for ( x = 0; x < net->num_ff_nodes; x++ ) {
        info = net->ff_nodes[x]->output_pins[0]->ace_info;

        // Odin process each vector 2 times, need to adjust count.
        info->num_ones = info->num_ones / 2;

        // Static Probability = NUMBER of ONES / Cycles
        info->static_prob = info->num_ones / (double) max_cycles;

        // Switching Probability = NUMBER of TOGGLES / Cycles
        info->switch_prob = info->num_toggles / (double) max_cycles;

        // Switch Activity = same as Switching Probability for FF nodes
        info->switch_act  = info->switch_prob;
    }

    // Process Internal Nodes
    for ( x = 0; x < net->num_internal_nodes; x++ ) {
        for ( y = 0; y < net->internal_nodes[x]->num_output_pins; y++ ) {
            info = net->internal_nodes[x]->output_pins[y]->ace_info;

            // Odin process each vector 2 times, need to adjust count.
            info->num_ones = info->num_ones / 2;

            // Static Probability = NUMBER of ONES / Cycles
            info->static_prob = info->num_ones / (double) max_cycles;

            // Switching Probability = NUMBER of TOGGLES / Cycles
            info->switch_prob = info->num_toggles / (double) max_cycles;
        }
    }

    //Compute Switching Activities for Internal Nodes
    compute_switching_activities ( net );

    // Output ACE Info
    output_ace_info (net, act_out );
}


/*---------------------------------------------------------------------------------------------
 * (function: compute_switching_activities )
 *
 * Calculates switchin activities for a given netlist
 *
 *-------------------------------------------------------------------------------------------*/
void compute_switching_activities ( netlist_t *net ) {

    int x,y,z;
    int n0, n1;
    ace_obj_info_t *info, *fanin_info;
    int d;
    DdManager * dd;
    DdNode * bdd;
    ace_cube_t * cube;

    // Initialize Cudd
    dd = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );

    // Process each internal node
    for ( x = 0; x < net->num_internal_nodes; x++ ) {
        // Process each output pin of node
        for ( y = 0; y < net->internal_nodes[x]->num_output_pins; y++ ) {
            info = net->internal_nodes[x]->output_pins[y]->ace_info;

            d = info->depth;
            d = (int) d * 0.4;
            if (d < 1) {
		d = 1;
            }

            // If nodes has a bitmap/truthtable use LAG 1 BDD model to determine switching activity
            if ( net->internal_nodes[x]->bit_map_line_count > 0 ) {
                bdd = build_bdd_for_node (dd, net->internal_nodes[x] );
                Cudd_Ref ( bdd );
                n0 = n1 = 0;
                ace_bdd_count_paths(dd, bdd, &n1, &n0);

                // Process each fanin node
                for ( z = 0;  z < net->internal_nodes[x]->num_input_pins; z++ ) {
                    fanin_info = net->internal_nodes[x]->input_pins[z]->net->driver_pin->ace_info;

                    fanin_info->prob0to1 = ACE_P0TO1 (fanin_info->static_prob, fanin_info->switch_prob / (double) d);
                    fanin_info->prob1to0 = ACE_P1TO0 (fanin_info->static_prob, fanin_info->switch_prob / (double) d);

                    prob_epsilon_fix(&fanin_info->prob0to1);
                    prob_epsilon_fix(&fanin_info->prob1to0);
                }

                cube = ace_cube_new_dc(net->internal_nodes[x]->num_input_pins);
                info->switch_act = 2.0 * calc_switch_prob_recur ( dd, bdd, bdd, cube, net->internal_nodes[x], 1.0, n1 > n0 ) * (double) d;
            }
            // If nodes does not have a bitmap, use switching probability as switching activity
            else {
                info->switch_act  = info->switch_prob;
            }
        }
    }
    Cudd_Quit(dd);
}


/*---------------------------------------------------------------------------------------------
 * (function: prob_epsilon_fix )
 *
 * Adjust probabilty for machine epsilon rounding
 *
 *-------------------------------------------------------------------------------------------*/
void prob_epsilon_fix(double * d) {

    if (*d < 0) {
	*d = 0;
    } else if (*d > 1) {
	*d = 1.;
    }
}


/*---------------------------------------------------------------------------------------------
 * (function: output_ace_info )
 *
 * Outputs ACE information for a given NetList
 *
 *-------------------------------------------------------------------------------------------*/
void output_ace_info (netlist_t *net, FILE *act_out ) {
    int x,y;

    // Process Top Input Nodes
    for ( x = 0; x < net->num_top_input_nodes; x++ ) {
        output_ace_info_node (net->top_input_nodes[x]->name,
                              net->top_input_nodes[x]->output_pins[0]->ace_info,
                              act_out );
    }

    // Process FF Nodes
    for ( x = 0; x < net->num_ff_nodes; x++ ) {
        output_ace_info_node ( net->ff_nodes[x]->name,
                               net->ff_nodes[x]->output_pins[0]->ace_info,
                               act_out );
    }

    // Process Internal Nodes
    for ( x = 0; x < net->num_internal_nodes; x++ ) {
        for ( y = 0; y < net->internal_nodes[x]->num_output_pins; y++ ) {
            output_ace_info_node ( net->internal_nodes[x]->output_pins[y]->name,
                                   net->internal_nodes[x]->output_pins[y]->ace_info,
                                   act_out );
        }
    }
}


/*---------------------------------------------------------------------------------------------
 * (function: output_ace_info_node )
 *
 * Outputs ACE information for a given Node
 *
 *-------------------------------------------------------------------------------------------*/
void output_ace_info_node ( char *name, ace_obj_info_t *info, FILE *act_out ) {

    /*
    printf ( "%s %f %f %f DEBUG: ONES: %3d TOGGLES: %ld\n",
             name,
             info->static_prob,
             info->switch_prob ,
             info->switch_act,
             info->num_ones,
             info->num_toggles
             );

    */
    fprintf ( act_out, "%s %f %f %f\n",
              name,
              info->static_prob,
              info->switch_prob ,
              info->switch_act
              );
}


/*---------------------------------------------------------------------------
 * (function:  set_clear )
 *
 * make "r" the empty set of "size" elements
 *
 * This function is from Espresso-mv
 *
 *---------------------------------------------------------------------------*/
pset set_clear(pset r, int size)
{
    int i = LOOPINIT(size);
    *r = i; do r[i] = 0; while (--i > 0);
    return r;
}


/*---------------------------------------------------------------------------------------------
 * (function:  node_error )
 *
 *-------------------------------------------------------------------------------------------*/
int node_error(int code) {
    switch (code) {
    case 0:
        error_message(ACE,-1, -1, "%s", "node_get_cube: node does not have a function");
        /* NOTREACHED */
        break;

    case 1:
        error_message(ACE,-1, -1, "%s","node_get_cube: cube index out of bounds");
        /* NOTREACHED */
        break;

    case 2:
        error_message(ACE,-1, -1, "%s","node_get_literal: bad cube");
        /* NOTREACHED */
        break;

    case 4:
        error_message(ACE,-1, -1, "%s","foreach_fanin: node changed during generation");
        /* NOTREACHED */
        break;

    case 5:
        error_message(ACE,-1, -1, "%s","foreach_fanout: node changed during generation");
        /* NOTREACHED */
        break;

    default:
        error_message(ACE,-1, -1, "%s","error code unused");
        break;

    }
    return 0;
}


/*---------------------------------------------------------------------------------------------
 * (function:  ace_cube_dup )
 *
 *-------------------------------------------------------------------------------------------*/
ace_cube_t * ace_cube_dup(ace_cube_t * cube) {
    int i;
    ace_cube_t * cube_copy;

    cube_copy = (ace_cube_t * ) malloc(sizeof(ace_cube_t));
    cube_copy->static_prob = cube->static_prob;
    cube_copy->num_literals = cube->num_literals;
    cube_copy->cube = set_new (2 * cube->num_literals);
    for (i = 0; i < cube->num_literals; i++) {
        switch (node_get_literal (cube->cube, i)) {
        case ZERO:
            set_insert(cube_copy->cube, 2 * i);
            set_remove(cube_copy->cube, 2 * i + 1);
            break;
        case ONE:
            set_remove(cube_copy->cube, 2 * i);
            set_insert(cube_copy->cube, 2 * i + 1);
            break;
        case TWO:
            set_insert(cube_copy->cube, 2 * i);
            set_insert(cube_copy->cube, 2 * i + 1);
            break;
	default:
	    error_message(ACE,-1, -1, "%s","Bad literal.");
        }
    }
    return (cube_copy);
}


/*---------------------------------------------------------------------------------------------
 * (function:  ace_cube_new_dc )
 *
 *-------------------------------------------------------------------------------------------*/
ace_cube_t * ace_cube_new_dc(int num_literals) {
    int i;
    ace_cube_t * new_cube;

    new_cube = (ace_cube_t * ) malloc(sizeof(ace_cube_t));
    new_cube->num_literals = num_literals;
    new_cube->static_prob = 1.0;
    new_cube->cube = set_new (2 * num_literals);

    for (i = 0; i < num_literals; i++) {
        set_insert(new_cube->cube, 2 * i);
        set_insert(new_cube->cube, 2 * i + 1);
    }

    return (new_cube);
}


/*---------------------------------------------------------------------------------------------
 * (function: build_bdd_for_node )
 *
 * Returns a Binary Decision Diagram (bdd) for a given node base upon its truth table entries.
 *
 * This function is based upon ABC's Abc_ConvertSopToBdd.
 *
 *-------------------------------------------------------------------------------------------*/
DdNode * build_bdd_for_node ( DdManager * dd, nnode_t *node ) {

    DdNode * bSum, * bCube, * bTemp, * bVar;
    int Value, x, y;

    bSum = Cudd_ReadLogicZero(dd);
    Cudd_Ref( bSum );

    // check the logic function of the node
    for ( x = 0; x < node->bit_map_line_count; x++ ) {
        bCube = Cudd_ReadOne(dd);   Cudd_Ref( bCube );
        for ( y = 0;  y < (int) strlen(node->bit_map[x]); y++ ) {
            Value = node->bit_map[x][y];
            if ( Value == '0' )
                bVar = Cudd_Not( Cudd_bddIthVar( dd, y ) );
            else if ( Value == '1' )
                bVar = Cudd_bddIthVar( dd, y );
            else
                continue;
            bCube  = Cudd_bddAnd( dd, bTemp = bCube, bVar );   Cudd_Ref( bCube );
            Cudd_RecursiveDeref( dd, bTemp );
        }
        bSum = Cudd_bddOr( dd, bTemp = bSum, bCube );
        Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    Cudd_Deref( bSum );
    return bSum;
}


/*---------------------------------------------------------------------------------------------
 * (function: ace_bdd_count_paths )
 *
 *-------------------------------------------------------------------------------------------*/
void ace_bdd_count_paths(DdManager * mgr, DdNode * bdd, int * num_one_paths, int * num_zero_paths) {
    DdNode * then_bdd;
    DdNode * else_bdd;

    if (bdd == Cudd_ReadLogicZero(mgr)) {
	*num_zero_paths = *num_zero_paths + 1;
	return;
    } else if (bdd == Cudd_ReadOne(mgr)) {
	*num_one_paths = *num_one_paths + 1;
	return;
    }

    then_bdd = Cudd_T(bdd);
    ace_bdd_count_paths(mgr, then_bdd, num_one_paths, num_zero_paths);

    else_bdd = Cudd_E(bdd);
    ace_bdd_count_paths(mgr, else_bdd, num_one_paths, num_zero_paths);
}


/*---------------------------------------------------------------------------------------------
 * (function: calc_cube_switch_prob_recur )
 *
 *-------------------------------------------------------------------------------------------*/
double calc_cube_switch_prob_recur(DdManager * mgr, DdNode * bdd, ace_cube_t * cube, nnode_t *node , st__table *visited, int phase ) {
    double * current_prob;
    short i;
    DdNode * bdd_if1, *bdd_if0;
    double then_prob, else_prob;
    ace_obj_info_t *fanin_info;

    if (bdd == Cudd_ReadLogicZero(mgr)) {
	if (phase == 0)
	    return (1.0);
	if (phase == 1)
	    return (0.0);
    } else if (bdd == Cudd_ReadOne(mgr)) {
	if (phase == 1)
	    return (1.0);
	if (phase == 0)
	    return (0.0);
    }

    if (st__lookup(visited, (char *) bdd, (char **) &current_prob)) {
	return (*current_prob);
    }

    i = Cudd_Regular(bdd)->index;

    current_prob = ( double * ) malloc(sizeof(double));

    if (Cudd_IsComplement(bdd)) {
	bdd_if1 = Cudd_E(bdd);
	bdd_if0 = Cudd_T(bdd);
    } else {
	bdd_if1 = Cudd_T(bdd);
	bdd_if0 = Cudd_E(bdd);
    }

    fanin_info =  node->input_pins[i]->net->driver_pin->ace_info;

    then_prob = calc_cube_switch_prob_recur(mgr, bdd_if1, cube, node, visited, phase );
    else_prob = calc_cube_switch_prob_recur(mgr, bdd_if0, cube, node, visited, phase );

    switch (node_get_literal (cube->cube, i)) {
    case ZERO:
	*current_prob = fanin_info->prob0to1 * then_prob
	    + (1.0 - fanin_info->prob0to1) * else_prob;
	break;
    case ONE:
	*current_prob = (1.0 - fanin_info->prob1to0) * then_prob
	    + fanin_info->prob1to0 * else_prob;
	break;
    case TWO:
	*current_prob = fanin_info->static_prob * then_prob
	    + (1.0 - fanin_info->static_prob) * else_prob;
	break;
    default:
	error_message(ACE,-1, -1, "%s","Bad literal.");
    }
    st__insert(visited, (char *) bdd, (char *) current_prob);

    return (*current_prob);
}


/*---------------------------------------------------------------------------------------------
 * (function: calc_cube_switch_prob )
 *
 *-------------------------------------------------------------------------------------------*/
double calc_cube_switch_prob(DdManager * mgr, DdNode * bdd, ace_cube_t * cube, nnode_t *node, int phase ) {
    double sp;
    st__table * visited;

    visited = (st__table * ) st__init_table( st__ptrcmp, st__ptrhash);

    sp = calc_cube_switch_prob_recur(mgr, bdd, cube, node,visited, phase );

    st__free_table(visited);

    return (sp);
}


/*---------------------------------------------------------------------------------------------
 * (function: calc_switch_prob_recur )
 *
 *-------------------------------------------------------------------------------------------*/
double calc_switch_prob_recur(DdManager * mgr, DdNode * bdd_next, DdNode * bdd, ace_cube_t * cube, nnode_t *node, double P1, int phase ) {

    short i;
    double switch_prob_t, switch_prob_e;
    double prob;
    DdNode * bdd_if1, *bdd_if0;
    ace_cube_t * cube0, *cube1;
    ace_obj_info_t  * info;

    if (bdd == Cudd_ReadLogicZero(mgr)) {
	if (phase != 1) {
	    return (0.0);
	}
	prob = calc_cube_switch_prob(mgr, bdd_next, cube, node, phase );
	prob *= P1;
	return (prob * P1);

    }
    else if (bdd == Cudd_ReadOne(mgr)) {
	if (phase != 0) {
	    return (0.0);
	}
	prob = calc_cube_switch_prob(mgr, bdd_next, cube, node, phase );
	prob *= P1;
	return (prob * P1);
    }

    /* Get literal index for this bdd node. */
    i = Cudd_Regular(bdd)->index;

    info =  node->input_pins[i]->net->driver_pin->ace_info;

    if (Cudd_IsComplement(bdd)) {
	bdd_if1 = Cudd_E(bdd);
	bdd_if0 = Cudd_T(bdd);
    } else {
	bdd_if1 = Cudd_T(bdd);
	bdd_if0 = Cudd_E(bdd);
    }

    /* Recursive call down the THEN branch */
    cube1 = ace_cube_dup(cube);
    set_remove(cube1->cube, 2 * i);
    set_insert(cube1->cube, 2 * i + 1);
    switch_prob_t = calc_switch_prob_recur(mgr, bdd_next, bdd_if1, cube1, node,
					   P1 * info->static_prob, phase );
    vtr::free(cube1->cube);
    vtr::free(cube1);
    /* Recursive call down the ELSE branch */
    cube0 = ace_cube_dup(cube);
    set_insert(cube0->cube, 2 * i);
    set_remove(cube0->cube, 2 * i + 1);
    switch_prob_e = calc_switch_prob_recur(mgr, bdd_next, bdd_if0,  cube0, node,
					   P1 * (1.0 - info->static_prob), phase );
    vtr::free(cube0->cube);
    vtr::free(cube0);

    return (switch_prob_t + switch_prob_e);
}
