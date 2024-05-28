
#include "read_xml_arch_file_noc_tag.h"

#include "read_xml_util.h"
#include "pugixml_util.hpp"
#include "vtr_log.h"
#include "arch_error.h"

/**
 * @brief Process the <topology> tag under <noc> tag.
 *
 * Using <topology> tag, the user can specify a custom
 * topology.
 *
 * @param topology_tag  An XML node pointing to a <topology> tag.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 * @param noc_ref To be filled with NoC router locations and their connectivity.
 */
static void process_topology(pugi::xml_node topology_tag,
                            const pugiutil::loc_data& loc_data,
                            t_noc_inf* noc_ref);

/**
 * @brief Process a <router> tag under a <topology> tag.
 *
 * A <topology> tag contains multiple <router> tags. Each <router> tag has
 * attributes that specify its location and connectivity to other NoC routers.
 *
 * @param router_tag An XML node pointing to a <router> tag.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 * @param noc_ref To be filled with the given router's location and connectivity information.
 * @param routers_in_arch_info Stores router information that includes the number of connections
 * a router has within a given topology and also the number of times a router was declared
 * in the arch file using the <router> tag. [router_id, [n_declarations, n_connections]]
 */
static void process_router(pugi::xml_node router_tag,
                           const pugiutil::loc_data& loc_data,
                           t_noc_inf* noc_ref,
                           std::map<int, std::pair<int, int>>& routers_in_arch_info);

/**
 * @brief Processes the <mesh> tag under <noc> tag.
 *
 * Using the <mesh> tag, the user can describe a NoC with
 * mesh topology.
 *
 * @param mesh_topology_tag An XML tag pointing to a <mesh> tag.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 * @param noc_ref To be filled with NoC router locations and their connectivity.
 */
static void process_mesh_topology(pugi::xml_node mesh_topology_tag,
                                  const pugiutil::loc_data& loc_data, t_noc_inf* noc_ref);


/**
 * Create routers and set their properties so that a mesh grid of routers is created.
 * Then connect the routers together so that a mesh topology is created.
 *
 * @param mesh_topology_tag An XML tag pointing to a <mesh> tag.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 * @param noc_ref To be filled with NoC router locations and their connectivity.
 * @param mesh_region_start_x The location the bottom left NoC router on the X-axis.
 * @param mesh_region_end_x The location the top right NoC router on the X-axis.
 * @param mesh_region_start_y The location the bottom left NoC router on the Y-axis.
 * @param mesh_region_end_y The location the top right NoC router on the Y-axis.
 * @param mesh_size The number of NoC routers in each row or column.
 */
static void generate_noc_mesh(pugi::xml_node mesh_topology_tag,
                              const pugiutil::loc_data& loc_data,
                              t_noc_inf* noc_ref,
                              double mesh_region_start_x, double mesh_region_end_x,
                              double mesh_region_start_y, double mesh_region_end_y,
                              int mesh_size);

/**
 * @brief Verify each router in the noc by checking whether they satisfy the following conditions:
 * - The router has only one declaration in the arch file
 * - The router has at least one connection to another router
 * If any of the conditions above are not met, then an error is thrown.
 *
 * @param routers_in_arch_info Stores router information that includes the number of connections
 * a router has within a given topology and also the number of times a router was declared
 * in the arch file using the <router> tag. [router_id, [n_declarations, n_connections]]
 */
static void verify_noc_topology(const std::map<int, std::pair<int, int>>& routers_in_arch_info);

/**
 * @brief Parses the connections field in <router> tag under <topology> tag.
 * The user provides the list of routers any given router is connected to
 * by the router ids seperated by spaces. For example:
 * connections="1 2 3 4 5"
 * Go through the connections here and store them. Also make sure the list is legal.
 *
 * @param router_tag An XML node referring to a <router> tag.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 * @param router_id The id field of the given <router> tag.
 * @param connection_list Parsed connections of the given <router>. To be filled by this
 * function.
 * @param connection_list_attribute_value Raw connections field of the given <router>.
 * @param routers_in_arch_info Stores router information that includes the number of connections
 * a router has within a given topology and also the number of times a router was declared
 * in the arch file using the <router> tag. [router_id, [n_declarations, n_connections]]
 *
 * @return True if parsing the connection list was successful.
 */
static bool parse_noc_router_connection_list(pugi::xml_node router_tag,
                                             const pugiutil::loc_data& loc_data,
                                             int router_id,
                                             std::vector<int>& connection_list,
                                             const std::string& connection_list_attribute_value,
                                             std::map<int, std::pair<int, int>>& routers_in_arch_info);

/**
 * @brief Each router needs a separate <router> tag in the architecture description file
 * to declare it. The number of declarations for each router in the
 * architecture file is updated here. Additionally, for any given topology,
 * a router can connect to other routers. The number of connections for each router
 * is also updated here.
 *
 * @param router_id The identifier of the router whose information needs to be updated.
 * @param router_updated_as_a_connection Indicated whether the router id was seen in the
 * connections field of a <router> tag.
 * @param routers_in_arch_info Stores router information that includes the number of connections
 * a router has within a given topology and also the number of times a router was declared
 * in the arch file using the <router> tag. [router_id, [n_declarations, n_connections]]
 */
static void update_router_info_in_arch(int router_id,
                                       bool router_updated_as_a_connection,
                                       std::map<int, std::pair<int, int>>& routers_in_arch_info);

/**
 * @brief Process <overrides> tag under <noc> tag.
 *
 * The user can override the default latency and bandwidth values
 * for specific NoC routers and links using <router> and <link>
 * tags under <overrides> tag.
 *
 * @param noc_overrides_tag An XML node pointing to a <overrides> tag.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 * @param noc_ref To be filled with parsed overridden latencies and bandwidths.
 */
static void process_noc_overrides(pugi::xml_node noc_overrides_tag,
                                  const pugiutil::loc_data& loc_data,
                                  t_noc_inf& noc_ref);

void process_noc_tag(pugi::xml_node noc_tag,
                     t_arch* arch,
                     const pugiutil::loc_data& loc_data) {
    // a vector representing all the possible attributes within the noc tag
    const std::vector<std::string> expected_noc_attributes = {"link_bandwidth", "link_latency", "router_latency", "noc_router_tile_name"};

    const std::vector<std::string> expected_noc_children_tags = {"mesh", "topology"};


    // identifier that lets us know when we could not properly convert a string conversion value
    std::string attribute_conversion_failure_string;

    // if we are here, then the user has a NoC in their architecture, so need to add it
    arch->noc = new t_noc_inf;
    t_noc_inf* noc_ref = arch->noc;

    /* process the noc attributes first */

    // quick error check to make sure that we don't have unexpected attributes
    pugiutil::expect_only_attributes(noc_tag, expected_noc_attributes, loc_data);

    // now go through and parse the required attributes for noc tag

    // variables below temporarily store the attribute values as string
    // this is so that scientific notation can be used for these attributes

    auto link_bandwidth_intermediate_val = pugiutil::get_attribute(noc_tag, "link_bandwidth", loc_data, pugiutil::REQUIRED).as_string();
    noc_ref->link_bandwidth = std::atof(link_bandwidth_intermediate_val);

    auto link_latency_intermediate_val = pugiutil::get_attribute(noc_tag, "link_latency", loc_data, pugiutil::REQUIRED).as_string();
    noc_ref->link_latency = std::atof(link_latency_intermediate_val);

    auto router_latency_intermediate_val = pugiutil::get_attribute(noc_tag, "router_latency", loc_data, pugiutil::REQUIRED).as_string();
    noc_ref->router_latency = std::atof(router_latency_intermediate_val);

    noc_ref->noc_router_tile_name = pugiutil::get_attribute(noc_tag, "noc_router_tile_name", loc_data, pugiutil::REQUIRED).as_string();

    // the noc parameters can only be non-zero positive values
    if ((noc_ref->link_bandwidth <= 0.) || (noc_ref->link_latency <= 0.) || (noc_ref->router_latency <= 0.)) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(noc_tag),
                       "The link bandwidth, link latency and router latency for the NoC must be a positive non-zero value.");
    }

    // check that the router tile name was supplied properly
    if (!(noc_ref->noc_router_tile_name.compare(attribute_conversion_failure_string))) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(noc_tag),
                       "The noc router tile name must be a string.");
    }

    /* We processed the NoC node, so now process the topology*/

    // make sure that only the topology tag is found under NoC
    pugiutil::expect_only_children(noc_tag, expected_noc_children_tags, loc_data);

    pugi::xml_node noc_mesh_topology = pugiutil::get_single_child(noc_tag, "mesh", loc_data, pugiutil::OPTIONAL);

    // we cannot check for errors related to number of routers and as well as whether a router is out of bounds (this will be done later)
    // the chip still needs to be sized

    if (noc_mesh_topology) {
        process_mesh_topology(noc_mesh_topology, loc_data, noc_ref);
    } else {
        pugi::xml_node noc_topology = pugiutil::get_single_child(noc_tag, "topology", loc_data, pugiutil::REQUIRED);

        process_topology(noc_topology, loc_data, noc_ref);
    }

    pugi::xml_node noc_overrides = pugiutil::get_single_child(noc_tag, "overrides", loc_data, pugiutil::OPTIONAL);
    if (noc_overrides) {
        process_noc_overrides(noc_overrides, loc_data, *noc_ref);
    }
}

/*
 * A NoC mesh is created based on the user supplied size and region location.
 */
static void process_mesh_topology(pugi::xml_node mesh_topology_tag,
                                  const pugiutil::loc_data& loc_data,
                                  t_noc_inf* noc_ref) {

    // identifier that lets us know when we could not properly convert an attribute value to a number
    constexpr int ATTRIBUTE_CONVERSION_FAILURE = -1;

    // a list of attributes that should be found for the mesh tag
    std::vector<std::string> expected_router_attributes = {"startx", "endx", "starty", "endy", "size"};

    // verify that only the acceptable attributes were supplied
    pugiutil::expect_only_attributes(mesh_topology_tag, expected_router_attributes, loc_data);

    // go through the attributes and store their values
    double mesh_region_start_x = pugiutil::get_attribute(mesh_topology_tag, "startx", loc_data, pugiutil::REQUIRED).as_double(ATTRIBUTE_CONVERSION_FAILURE);
    double mesh_region_end_x = pugiutil::get_attribute(mesh_topology_tag, "endx", loc_data, pugiutil::REQUIRED).as_double(ATTRIBUTE_CONVERSION_FAILURE);
    double mesh_region_start_y = pugiutil::get_attribute(mesh_topology_tag, "starty", loc_data, pugiutil::REQUIRED).as_double(ATTRIBUTE_CONVERSION_FAILURE);
    double mesh_region_end_y = pugiutil::get_attribute(mesh_topology_tag, "endy", loc_data, pugiutil::REQUIRED).as_double(ATTRIBUTE_CONVERSION_FAILURE);

    int mesh_size = pugiutil::get_attribute(mesh_topology_tag, "size", loc_data, pugiutil::REQUIRED).as_int(ATTRIBUTE_CONVERSION_FAILURE);

    // verify that the attributes provided were legal
    if ((mesh_region_start_x < 0) || (mesh_region_end_x < 0) || (mesh_region_start_y < 0) || (mesh_region_end_y < 0) || (mesh_size < 0)) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(mesh_topology_tag),
                       "The parameters for the mesh topology have to be positive values.");
    }

    // now create the mesh topology for the noc
    // create routers, make connections and determine positions
    generate_noc_mesh(mesh_topology_tag, loc_data, noc_ref, mesh_region_start_x, mesh_region_end_x, mesh_region_start_y, mesh_region_end_y, mesh_size);
}

static void generate_noc_mesh(pugi::xml_node mesh_topology_tag,
                              const pugiutil::loc_data& loc_data,
                              t_noc_inf* noc_ref,
                              double mesh_region_start_x, double mesh_region_end_x,
                              double mesh_region_start_y, double mesh_region_end_y,
                              int mesh_size) {
    // check that the mesh size of the router is not 0
    if (mesh_size == 0) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(mesh_topology_tag),
                       "The NoC mesh size cannot be 0.");
    }

    // calculating the vertical and horizontal distances between routers in the supplied region
    // we decrease the mesh size by 1 when calculating the spacing so that the first and
    // last routers of each row or column are positioned on the mesh boundary
    /*
     * For example:
     * - If we had a mesh size of 3, then using 3 would result in a spacing that would result in
     * one router positions being placed in either the start of the region or end of the region.
     * This is because the distance calculation resulted in having 3 spaces between the ends of the region
     *
     * start              end
     ***   ***   ***   ***
     *
     * - if we instead used 2 in the distance calculation, the resulting positions would result in
     * having 2 routers positioned on the start and end of the region.
     * This is beacuse we now specified 2 spaces between the region and this allows us to place 2 routers
     * on the regions edges and one router in the center.
     *
     * start        end
     ***   ***   ***
     *
     * THe reasoning for this is to reduce the number of calculated router positions.
     */
    double vertical_router_separation = (mesh_region_end_y - mesh_region_start_y) / (mesh_size - 1);
    double horizontal_router_separation = (mesh_region_end_x - mesh_region_start_x) / (mesh_size - 1);

    t_router temp_router;

    // improper region check
    if ((vertical_router_separation <= 0) || (horizontal_router_separation <= 0)) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(mesh_topology_tag),
                       "The NoC region is invalid.");
    }

    // create routers and their connections
    // start with router id 0 (bottom left of the chip) to the maximum router id (top right of the chip)
    for (int j = 0; j < mesh_size; j++) {
        for (int i = 0; i < mesh_size; i++) {
            // assign router id
            temp_router.id = (mesh_size * j) + i;

            // calculate router position
            /* The first and last router of each column or row will be located on the mesh region boundary,
             * the remaining routers will be placed within the region and seperated from other routers
             * using the distance calculated previously.
             */
            temp_router.device_x_position = (i * horizontal_router_separation) + mesh_region_start_x;
            temp_router.device_y_position = (j * vertical_router_separation) + mesh_region_start_y;

            // assign connections
            // check if there is a router to the left
            if ((i - 1) >= 0) {
                // add the left router as a connection
                temp_router.connection_list.push_back((mesh_size * j) + i - 1);
            }

            // check if there is a router to the top
            if ((j + 1) <= (mesh_size - 1)) {
                // add the top router as a connection
                temp_router.connection_list.push_back((mesh_size * (j + 1)) + i);
            }

            // check if there is a router to the right
            if ((i + 1) <= (mesh_size - 1)) {
                // add the router located to the right
                temp_router.connection_list.push_back((mesh_size * j) + i + 1);
            }

            // check of there is a router below
            if ((j - 1) >= (0)) {
                // add the bottom router as a connection
                temp_router.connection_list.push_back((mesh_size * (j - 1)) + i);
            }

            // add the router to the list
            noc_ref->router_list.push_back(temp_router);

            // clear the current router information for the next router
            temp_router.connection_list.clear();
        }
    }
}

/*
 * Go through each router in the NoC and store the list of routers that connect to it.
 */
static void process_topology(pugi::xml_node topology_tag,
                            const pugiutil::loc_data& loc_data,
                            t_noc_inf* noc_ref) {
    // The topology tag should have no attributes, check that
    pugiutil::expect_only_attributes(topology_tag, {}, loc_data);

    /**
     * Stores router information that includes the number of connections a router
     * has within a given topology and also the number of times a router was declared
     * in the arch file using the <router> tag.
     * key --> router id
     * value.first  --> the number of router declarations
     * value.second --> the number of router connections
     * This is used only for error checking.
     */
    std::map<int, std::pair<int, int>> routers_in_arch_info;

    /* Now go through the children tags of topology, which is basically
     * each router found within the NoC
     */
    for (pugi::xml_node router : topology_tag.children()) {
        // we can only have router tags within the topology
        if (router.name() != std::string("router")) {
            bad_tag(router, loc_data, topology_tag, {"router"});
        } else {
            // current tag is a valid router, so process it
            process_router(router, loc_data, noc_ref, routers_in_arch_info);
        }
    }

    // check whether any routers were supplied
    if (noc_ref->router_list.empty()) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(topology_tag),
                       "No routers were supplied for the NoC.");
    }

    // check that the topology of the noc was correctly described in the arch file
    verify_noc_topology(routers_in_arch_info);
}

/*
 * Store the properties of a single router and then store the list of routers that connect to it.
 */
static void process_router(pugi::xml_node router_tag,
                           const pugiutil::loc_data& loc_data,
                           t_noc_inf* noc_ref,
                           std::map<int, std::pair<int, int>>& routers_in_arch_info) {
    // identifier that lets us know when we could not properly convert an attribute value to an integer
    constexpr int ATTRIBUTE_CONVERSION_FAILURE = -1;

    // an accepted list of attributes for the router tag
    std::vector<std::string> expected_router_attributes = {"id", "positionx", "positiony", "connections"};

    // check that only the accepted router attributes are found in the tag
    pugiutil::expect_only_attributes(router_tag, expected_router_attributes, loc_data);

    // variable to store current router info
    t_router router_info;

    // store the router information from the attributes
    router_info.id = pugiutil::get_attribute(router_tag, "id", loc_data, pugiutil::REQUIRED).as_int(ATTRIBUTE_CONVERSION_FAILURE);

    router_info.device_x_position = pugiutil::get_attribute(router_tag, "positionx", loc_data, pugiutil::REQUIRED).as_double(ATTRIBUTE_CONVERSION_FAILURE);

    router_info.device_y_position = pugiutil::get_attribute(router_tag, "positiony", loc_data, pugiutil::REQUIRED).as_double(ATTRIBUTE_CONVERSION_FAILURE);

    // verify whether the attribute information was legal
    if ((router_info.id < 0) || (router_info.device_x_position < 0) || (router_info.device_y_position < 0)) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(router_tag),
                       "The router id, and position (x & y) for the router must be a positive number.");
    }

    // get the current router connection list
    std::string router_connection_list_attribute_value = pugiutil::get_attribute(router_tag, "connections", loc_data, pugiutil::REQUIRED).as_string();

    // if the connections attribute was not provided, or it was empty, then we don't process it and throw a warning
    if (!router_connection_list_attribute_value.empty()) {
        // process the router connection list
        bool router_connection_list_result = parse_noc_router_connection_list(router_tag, loc_data, router_info.id,
                                                                              router_info.connection_list,
                                                                              router_connection_list_attribute_value,
                                                                              routers_in_arch_info);

        // check if the user provided a legal router connection list
        if (!router_connection_list_result) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(router_tag),
                           "The 'connections' attribute for the router must be a list of integers seperated by spaces, "
                           "where each integer represents a router id that the current router is connected to.");
        }

    } else {
        VTR_LOGF_WARN(loc_data.filename_c_str(), loc_data.line(router_tag),
                      "The router with id:%d either has an empty 'connections' attribute "
                      "or does not have any associated connections to other routers in the NoC.\n",
                      router_info.id);
    }

    // at this point the current router information was completely legal, so we store the newly created router within the noc
    noc_ref->router_list.push_back(router_info);

    // update the number of declarations info for the current router (since we just finished processing one <router> tag)
    update_router_info_in_arch(router_info.id, false, routers_in_arch_info);
}

static void verify_noc_topology(const std::map<int, std::pair<int, int>>& routers_in_arch_info) {

    for (const auto& [router_id, router_info] : routers_in_arch_info) {
        const auto [n_declarations, n_connections] = router_info;
        // case where the router was included in the architecture and had no connections to other routers
        if (n_declarations == 1 && n_connections == 0) {
            archfpga_throw("", -1,
                           "The router with id:'%d' is not connected to any other router in the NoC.",
                           router_id);

        } // case where a router was found to be connected to another router but not declared using the <router> tag in the arch file (i.e. missing)
        else if (n_declarations == 0 && n_connections > 0) {
            archfpga_throw("", -1,
                           "The router with id:'%d' was found to be connected to another router "
                           "but missing in the architecture file. Add the router using the <router> tag.",
                           router_id);

        } // case where the router was declared multiple times in the architecture file (multiple <router> tags for the same router)
        else if (n_declarations > 1) {
            archfpga_throw("", -1,
                           "The router with id:'%d' was included more than once in the architecture file. "
                           "Routers should only be declared once.",
                           router_id);
        }
    }
}

static bool parse_noc_router_connection_list(pugi::xml_node router_tag,
                                             const pugiutil::loc_data& loc_data,
                                             int router_id,
                                             std::vector<int>& connection_list,
                                             const std::string& connection_list_attribute_value,
                                             std::map<int, std::pair<int, int>>& routers_in_arch_info) {
    // we wil be modifying the string so store it in a temporary variable
    // additionally, we process substrings seperated by spaces,
    // so we add a space at the end of the string to be able to process the last sub-string
    std::string modified_attribute_value = connection_list_attribute_value + " ";
    const std::string delimiter = " ";
    std::stringstream single_connection;
    int converted_connection;

    size_t position = 0;

    bool result = true;

    // find the position of the first space in the connection list string
    while ((position = modified_attribute_value.find(delimiter)) != std::string::npos) {
        // the string upto the space represent a single connection, so grab the substring
        single_connection << modified_attribute_value.substr(0, position);

        // convert the connection to an integer
        single_connection >> converted_connection;

        /* we expect the connection list to be a string of integers seperated by spaces,
         * where each integer represents a router id that the current router is connected to.
         * So we make sure that the router id was an integer.
         */
        if (single_connection.fail()) {
            // if we are here, then an integer was not supplied
            result = false;
            break;
        }

        // check the case where a duplicate connection was provided
        if (std::find(connection_list.begin(), connection_list.end(), converted_connection) != connection_list.end()) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(router_tag),
                           "The router with id:'%d' was included multiple times in the connection list for another router.", converted_connection);
        }

        // make sure that the current router isn't connected to itself
        if (router_id == converted_connection) {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(router_tag),
                           "The router with id:%d was added to its own connection list. A router cannot connect to itself.", router_id);
        }

        // if we are here then a legal router id was supplied, so store it
        connection_list.push_back(converted_connection);
        // update the connection information for the current router in the connection list
        update_router_info_in_arch(converted_connection, true, routers_in_arch_info);

        // before we process the next router connection, we need to delete the substring (current router connection)
        modified_attribute_value.erase(0, position + delimiter.length());
        // clear the buffer that stores the router connection in a string format for the next iteration
        single_connection.clear();
    }

    return result;
}

static void update_router_info_in_arch(int router_id,
                                       bool router_updated_as_a_connection,
                                       std::map<int, std::pair<int, int>>& routers_in_arch_info) {
    // try to add the router
    // if it was already added, get the existing one
    auto [curr_router_info, _] = routers_in_arch_info.insert({router_id, {0, 0}});

    auto& [n_declarations, n_connections] = curr_router_info->second;

    // case where the current router was provided while parsing the connections of another router
    if (router_updated_as_a_connection) {
        // since we are within the case where the current router is being processed as
        // a connection to another router we just increment its number of connections
        n_connections++;

    } else {
        // since we are within the case where the current router is processed from a <router> tag,
        // we just increment its number of declarations
        n_declarations++;
    }
}

static void process_noc_overrides(pugi::xml_node noc_overrides_tag,
                                  const pugiutil::loc_data& loc_data,
                                  t_noc_inf& noc_ref) {
    // an accepted list of attributes for the router tag
    const std::vector<std::string> expected_router_attributes{"id", "latency"};
    // the list of expected values for link tag
    const std::vector<std::string> expected_link_override_attributes{"src", "dst", "latency", "bandwidth"};

    for (pugi::xml_node override_tag : noc_overrides_tag.children()) {
        std::string_view override_name = override_tag.name();

        if (override_name == "router") {
            // check that only the accepted router attributes are found in the tag
            pugiutil::expect_only_attributes(override_tag, expected_router_attributes, loc_data);

            // store the router information from the attributes
            int id = pugiutil::get_attribute(noc_overrides_tag, "id", loc_data, pugiutil::REQUIRED).as_int(-1);

            auto router_latency_override = pugiutil::get_attribute(override_tag, "latency", loc_data, pugiutil::REQUIRED).as_string();
            double latency = std::atof(router_latency_override);

            if (id < 0 || latency <= 0.0) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                               "The latency override value (%g) for router with id:%d is not legal. "
                               "The router id must be non-negative and the latency must be positive.",
                               latency, id);
            }

            auto it = std::find_if(noc_ref.router_list.begin(), noc_ref.router_list.end(), [id](const t_router& router) {
                return router.id == id;
            });

            if (it == noc_ref.router_list.end()) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                               "The router with id:%d could not be found in the topology.",
                               id);
            }

            auto [_, success] = noc_ref.router_latency_overrides.insert({id, latency});
            if (!success) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                               "The latency of the router with id:%d wad overridden once before.",
                               id);
            }

        } else if (override_name == "link") {
            // check that only the accepted link attributes are found in the tag
            pugiutil::expect_only_attributes(override_tag, expected_link_override_attributes, loc_data);

            // store the router information from the attributes
            int src = pugiutil::get_attribute(noc_overrides_tag, "src", loc_data, pugiutil::REQUIRED).as_int(-1);
            int dst = pugiutil::get_attribute(noc_overrides_tag, "dst", loc_data, pugiutil::REQUIRED).as_int(-1);

            if (src < 0 || dst < 0) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                               "The source and destination router ids (%d, %d) must be non-negative.",
                               src, dst);
            }

            auto it = std::find_if(noc_ref.router_list.begin(), noc_ref.router_list.end(), [src, dst](const t_router& router) {
                return router.id == src &&
                       std::find(router.connection_list.begin(), router.connection_list.end(), dst) != router.connection_list.end();
            });

            if (it == noc_ref.router_list.end()) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                               "There is no links from the router with id:%d to the router with id:%d.",
                               src, dst);
            }

            auto link_latency_override = pugiutil::get_attribute(override_tag, "latency", loc_data, pugiutil::REQUIRED).as_string(nullptr);
            if (link_latency_override != nullptr) {
                double latency = std::atof(link_latency_override);
                if (latency <= 0.0) {
                    archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                                   "The override link latency value for link (%d, %d) must be positive:%g." ,
                                   src, dst, latency);
                }

                auto [_, success] = noc_ref.link_latency_overrides.insert({{src, dst}, latency});
                if (!success) {
                    archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                                   "The latency for link (%d, %d) was overridden once before." ,
                                   src, dst);
                }
            }

            auto link_bandwidth_override = pugiutil::get_attribute(override_tag, "bandwidth", loc_data, pugiutil::REQUIRED).as_string(nullptr);
            if (link_bandwidth_override != nullptr) {
                double bandwidth = std::atof(link_latency_override);
                if (bandwidth <= 0.0) {
                    archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                                   "The override link bandwidth value for link (%d, %d) must be positive:%g." ,
                                   src, dst, bandwidth);
                }

                auto [_, success] = noc_ref.link_bandwidth_overrides.insert({{src, dst}, bandwidth});
                if (!success) {
                    archfpga_throw(loc_data.filename_c_str(), loc_data.line(override_tag),
                                   "The bandwidth for link (%d, %d) was overridden once before." ,
                                   src, dst);
                }
            }
        } else {
            bad_tag(override_tag, loc_data, noc_overrides_tag, {"router", "link"});
        }
    }
}