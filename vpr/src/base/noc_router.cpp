#include "noc_router.h"

// constructor
NocRouter::NocRouter(int id, int grid_position_x, int grid_position_y)
    : router_user_id(id)
    , router_grid_position_x(grid_position_x)
    , router_grid_position_y(grid_position_y) {
    // initialize variables
    router_block_ref = ClusterBlockId(0);
}

// getters
int NocRouter::get_router_user_id(void) const {
    return router_user_id;
}

int NocRouter::get_router_grid_position_x(void) const {
    return router_grid_position_x;
}

int NocRouter::get_router_grid_position_y(void) const {
    return router_grid_position_y;
}

ClusterBlockId NocRouter::get_router_block_ref(void) const {
    return router_block_ref;
}

// setters
void NocRouter::set_router_block_ref(ClusterBlockId router_block_ref_id) {
    router_block_ref = router_block_ref_id;
    return;
}