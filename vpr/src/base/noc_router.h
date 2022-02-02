#ifndef NOC_ROUTER_H
#define NOC_ROUTER_H

#include <iostream>
#include <string>

// this represents a physical router on the chip

class NocRouter {

    private:
        // this represents the id provided by the user when describing
        // the NoC in the architecture description file 
        int router_id; 
        int router_grid_position_x;
        int router_grid_position_y;

        // atom id and clustering block id can be used to identigy which router moduleswe are using
        // atom id is faster than string
        std::string router_design_module_ref;

        // traffic flow information will be providedin an input file through
        // module names and how the trffic flows between them

    public:
        NocRouter(int id, int grid_position_x, int grid_position_y);

        int get_router_id(void) const;
        int get_router_grid_position_x(void) const;
        int get_router_grid_position_y(void) const;

        std::string get_router_design_module_ref(void) const;

        // setters
        void set_router_design_module_ref(std::string design_module_ref);



};









#endif