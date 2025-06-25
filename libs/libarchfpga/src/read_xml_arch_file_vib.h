#include <vector>
#include "physical_types.h"

#include "pugixml.hpp"
#include "pugixml_util.hpp"

void process_vib_arch(pugi::xml_node Parent, std::vector<t_physical_tile_type>& PhysicalTileTypes, t_arch* arch, const pugiutil::loc_data& loc_data);

void process_vib_layout(pugi::xml_node vib_layout_tag, t_arch* arch, const pugiutil::loc_data& loc_data);

t_vib_grid_def process_vib_grid_layout(vtr::string_internment& strings, pugi::xml_node layout_type_tag, const pugiutil::loc_data& loc_data, t_arch* arch, int& num_of_avail_layer);