#ifndef ROUTER_H
#define ROUTER_H

#include <iostream>
#include <string>

class Router {

    private:
        int router_id;
        int router_grid_position_x;
        int router_grid_position_y;

        std::string router_design_module_name;

    public:
        Router(int id, int grid_position_x, int grid_position_y);

        int get_router_id(void);
        int get_router_grid_position_x(void);
        int get_router_grid_position_y(void);

        std::string get_router_design_module_name(void);

        // setters
        void set_router_design_module_name(std::string design_module_name);



};









#endif