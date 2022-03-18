#include <cstring>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <experimental/filesystem>

#include <sys/stat.h>

#include "tb_build_switchblocks.h"

#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_string_view.h"

#include "vpr_error.h"

#include "build_switchblocks.h"
#include "build_switchblocks.cpp"
#include "physical_types.h"
#include "parse_switchblocks.h"
#include "vtr_expr_eval.h"
#include <fstream>
using vtr::FormulaParser;
using vtr::t_formula_data;

void scratchpad_init()
{

}

void tb_build_switchblocks()
{
    printf("'tb_build_switchblocks.cpp' is working\n");
}

void tb_compute_wire_connections()
{
    // //compute_wire_connections arguments initialisation
    // size_t x_coord = 0;
    // size_t y_coord = 0;
    // enum e_side from_side = TOP;
    // enum e_side to_side = TOP;
    // const t_chan_details& chan_details_x;
    // const t_chan_details& chan_details_y;
    // t_switchblock_inf sb;
    // const DeviceGrid& grid;
    // t_wire_type_sizes wire_type_sizes;
    // e_directionality directionality;
    // t_sb_connection_map* sb_conns = new t_sb_connection_map;
    // vtr::RandState& rand_state;
    // t_wireconn_scratchpad scratchpad;


    // //compute_wire_connections execution
    // compute_wire_connections(   x_coord, 
    //                             y_coord, 
    //                             from_side, 
    //                             to_side,
    //                             chan_details_x, 
    //                             chan_details_y, 
    //                             &sb, 
    //                             grid,
    //                             &wire_type_sizes, 
    //                             directionality, 
    //                             sb_conns, 
    //                             rand_state, 
    //                             &scratchpad);
}

void tb_compute_wireconn_connections()
{
    // /**********
    // compute_wireconn_connections arguments initialisation
    // **********/

    // //  grid initialisation
    // //vtr::Matrix<t_grid_tile> grid_tile[2][2] = {{},{}};
    // const DeviceGrid& grid;
    
    
    // e_directionality directionality;
    // const t_chan_details& from_chan_details;
    // const t_chan_details& to_chan_details;
    // Switchblock_Lookup sb_conn;
    // int from_x = 0;
    // int from_y = 0;
    // int to_x = 0;
    // int to_y = 0;
    // t_rr_type from_chan_type;
    // t_rr_type to_chan_type;
    // const t_wire_type_sizes* wire_type_sizes;
    // const t_switchblock_inf* sb;
    // t_wireconn_inf* wireconn_ptr;
    // t_sb_connection_map* sb_conns;
    // vtr::RandState& rand_state;
    // t_wireconn_scratchpad* scratchpad;

    // /**********
    // compute_wireconn_connections execution
    // **********/

    // compute_wireconn_connections(   grid, 
    //                                 directionality, 
    //                                 from_chan_details, 
    //                                 to_chan_details,
    //                                 sb_conn, 
    //                                 from_x, 
    //                                 from_y, 
    //                                 to_x, 
    //                                 to_y, 
    //                                 from_chan_type, 
    //                                 to_chan_type, 
    //                                 wire_type_sizes,
    //                                 sb, 
    //                                 wireconn_ptr, 
    //                                 sb_conns, 
    //                                 rand_state, 
    //                                 scratchpad);
}