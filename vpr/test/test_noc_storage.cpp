#include "catch2/catch_test_macros.hpp"

#include "noc_storage.h"

#include <random>

// controls how many routers are in the noc storage, when testing it
#define NUM_OF_ROUTERS 100

// defines the number of links each router has
// range [1, NUM_OF_ROUTERS - 1]
#define NOC_CONNECTIVITY 10

namespace {

constexpr double DUMMY_LATENCY = 1e-9;
constexpr double DUMMY_BANDWIDTH = 1e12;

TEST_CASE("test_adding_routers_to_noc_storage", "[vpr_noc]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_ROUTERS);

    // create a vector routers that we will use as the golden vector set
    std::vector<NocRouter> golden_set;

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    // testing data structure
    NocStorage test_noc;

    // store the reference to device grid with
    // this will be set to the total number of routers (and should be set before adding routers)
    test_noc.set_device_grid_spec((int)NUM_OF_ROUTERS, 0);

    NocRouterId converted_id;

    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number + dist(rand_num_gen);
        router_grid_position_y = router_number + dist(rand_num_gen);

        // add router to the golden vector
        golden_set.emplace_back(router_number, router_grid_position_x, router_grid_position_y, 0, DUMMY_LATENCY);

        // add tje router to the noc
        test_noc.add_router(curr_router_id, router_grid_position_x, router_grid_position_y, 0, DUMMY_LATENCY);
    }

    // now verify that the routers were added properly by reading the routers back from the noc and comparing them to the golden set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // get the converted router id and the corresponding router to verify
        // no need to run a conversion, the router id mapping is 1 to 1
        converted_id = (NocRouterId)router_number;
        const NocRouter& router_to_verify = test_noc.get_single_noc_router(converted_id);

        // compare all the router properties
        REQUIRE(golden_set[router_number].get_router_user_id() == router_to_verify.get_router_user_id());

        REQUIRE(golden_set[router_number].get_router_grid_position_x() == router_to_verify.get_router_grid_position_x());

        REQUIRE(golden_set[router_number].get_router_grid_position_y() == router_to_verify.get_router_grid_position_y());
    }
}
TEST_CASE("test_router_id_conversion", "[vpr_noc]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_ROUTERS);

    // create a vector routers that we will use as the golden vector set
    std::vector<NocRouter> golden_set;

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    // testing data structure
    NocStorage test_noc;

    // store the reference to device grid with
    // this will be set to the total number of routers (and should be set before adding routers)
    test_noc.set_device_grid_spec((int)NUM_OF_ROUTERS, 0);

    NocRouterId converted_id;

    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number + dist(rand_num_gen);
        router_grid_position_y = router_number + dist(rand_num_gen);

        // add router to the golden vector
        golden_set.emplace_back(router_number, router_grid_position_x, router_grid_position_y, 0, DUMMY_LATENCY);

        // add tje router to the noc
        test_noc.add_router(curr_router_id, router_grid_position_x, router_grid_position_y, 0, DUMMY_LATENCY);
    }

    // now verify that the routers were added properly by reading the routers back from the noc and comparing them to the golden set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // get the converted router id and the corresponding router to verify
        converted_id = test_noc.convert_router_id(router_number);
        const NocRouter& router_to_verify = test_noc.get_single_noc_router(converted_id);

        // compare all the router properties
        REQUIRE(golden_set[router_number].get_router_user_id() == router_to_verify.get_router_user_id());

        REQUIRE(golden_set[router_number].get_router_grid_position_x() == router_to_verify.get_router_grid_position_x());

        REQUIRE(golden_set[router_number].get_router_grid_position_y() == router_to_verify.get_router_grid_position_y());
    }
}
TEST_CASE("test_add_link", "[vpr_noc]") {
    // create a vector to store the golden links
    std::vector<NocLink> golden_set;

    // temp variables that hold the routers involved within a link
    NocRouterId source;
    NocRouterId sink;

    NocLinkId link_id;

    // testing data structure
    NocStorage test_noc;

    // store the reference to device grid with
    // this will be set to the total number of routers (and should be set before adding routers)
    test_noc.set_device_grid_spec((int)NUM_OF_ROUTERS, 0);

    // keeps track of the number of links created
    int total_num_of_links = 0;

    // noc router stuff (we need routers before being able to add links)
    int router_id = 0;
    int curr_router_x_pos = 0;
    int curr_router_y_pos = 0;

    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        router_id = router_number;

        // add tje router to the noc
        test_noc.add_router(router_id,
                            curr_router_x_pos,
                            curr_router_y_pos,
                            0,
                            DUMMY_LATENCY);
    }

    // allocate the size for outgoing link vector for each router
    test_noc.make_room_for_noc_router_link_list();
    // incremental counter used as NocLinkId
    int noc_link_id_counter = 0;

    for (int source_router_id = 0; source_router_id < NUM_OF_ROUTERS; source_router_id++) {
        source = (NocRouterId)source_router_id;

        for (int sink_router_id = 0; sink_router_id < NOC_CONNECTIVITY; sink_router_id++) {
            sink = (NocRouterId)sink_router_id;

            // makes sure we do not create a link for a router who acts as a sink and source
            if (source_router_id != sink_router_id) {
                // converting the counter to link index
                link_id = (NocLinkId)noc_link_id_counter;
                noc_link_id_counter++;

                // add link to the golden reference
                golden_set.emplace_back(link_id, source, sink, DUMMY_BANDWIDTH, DUMMY_LATENCY);

                // add the link to the NoC
                test_noc.add_link(source, sink, DUMMY_BANDWIDTH, DUMMY_LATENCY);

                total_num_of_links++;
            }
        }
    }

    // verify that the links were added properly to the NoC
    for (int link_number = 0; link_number < total_num_of_links; link_number++) {
        // converting current link to the index used by the NocStorage class
        link_id = (NocLinkId)link_number;
        // using the link index, get it from the NoC
        const NocLink& current_link_to_test = test_noc.get_single_noc_link(link_id);

        // now get the source and sink routers of the test link
        const NocRouter& test_link_source_router = test_noc.get_single_noc_router(current_link_to_test.get_source_router());
        const NocRouter& test_link_sink_router = test_noc.get_single_noc_router(current_link_to_test.get_sink_router());

        // now get the source and sink routers of the golden link
        const NocRouter& golden_link_source_router = test_noc.get_single_noc_router(golden_set[link_number].get_source_router());
        const NocRouter& golden_link_sink_router = test_noc.get_single_noc_router(golden_set[link_number].get_sink_router());

        // verify the test link by checking that the source and sink routers match the golden reference link
        REQUIRE(golden_link_source_router.get_router_user_id() == test_link_source_router.get_router_user_id());
        REQUIRE(golden_link_sink_router.get_router_user_id() == test_link_sink_router.get_router_user_id());
    }
}
TEST_CASE("test_router_link_list", "[vpr_noc]") {
    // create a vector to store the golden links
    vtr::vector<NocRouterId, std::vector<NocLinkId>> golden_set;

    golden_set.resize(NUM_OF_ROUTERS);

    // temp variables that hold the routers involved within a link
    NocRouterId source;
    NocRouterId sink;

    // testing data structure
    NocStorage test_noc;

    // store the reference to device grid with
    // this will be set to the total number of routers (and should be set before adding routers)
    test_noc.set_device_grid_spec((int)NUM_OF_ROUTERS, 0);

    // need to assign

    int curr_link_number = 0;

    int connection_size;

    // noc router stuff (we need routers before being able to add links)
    int router_id = 0;
    int curr_router_x_pos = 0;
    int curr_router_y_pos = 0;

    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        router_id = router_number;

        // add tje router to the noc
        test_noc.add_router(router_id, curr_router_x_pos, curr_router_y_pos, 0, DUMMY_LATENCY);
    }

    // allocate the size for outgoing link vector for each router
    test_noc.make_room_for_noc_router_link_list();

    for (int source_router_id = 0; source_router_id < NUM_OF_ROUTERS; source_router_id++) {
        source = (NocRouterId)source_router_id;

        for (int sink_router_id = 0; sink_router_id < NOC_CONNECTIVITY; sink_router_id++) {
            sink = (NocRouterId)sink_router_id;

            // makes sure we do not create a link for a router who acts as a sink and source
            if (source_router_id != sink_router_id) {
                // add the link to the NoC
                test_noc.add_link(source, sink, DUMMY_BANDWIDTH, DUMMY_LATENCY);

                // add the link id to the golden set
                golden_set[source].push_back((NocLinkId)curr_link_number);

                curr_link_number++;
            }
        }
    }

    // now verify that the connection lists were created correctly
    for (int id = 0; id < NUM_OF_ROUTERS; id++) {
        // get the current router id
        source = (NocRouterId)id;

        // get the router connections from the
        const std::vector<NocLinkId>& router_links = test_noc.get_noc_router_outgoing_links(source);

        // get the size of the current router connection list
        connection_size = golden_set[source].size();

        // go through the links from the noc and make sure they match the golden set
        for (int link_index = 0; link_index < connection_size; link_index++) {
            REQUIRE(golden_set[source][link_index] == router_links[link_index]);
        }
    }
}
TEST_CASE("test_remove_link", "[vpr_noc]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());

    // random number generation to determine routers of the link to remove
    std::uniform_int_distribution<std::mt19937::result_type> src_router(0, NUM_OF_ROUTERS - 1);
    std::uniform_int_distribution<std::mt19937::result_type> sink_router(1, NOC_CONNECTIVITY - 1);

    // create the NoC
    NocStorage test_noc;

    // store the reference to device grid with
    // this will be set to the total number of routers (and should be set before adding routers)
    test_noc.set_device_grid_spec((int)NUM_OF_ROUTERS, 0);

    // temp variables that hold the routers involved within a link
    NocRouterId source;
    NocRouterId sink;

    // create routers and add it to the
    // noc router stuff (we need routers before being able to add links)
    int router_id = 0;
    int curr_router_x_pos = 0;
    int curr_router_y_pos = 0;

    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        router_id = router_number;

        // add tje router to the noc
        test_noc.add_router(router_id,
                            curr_router_x_pos,
                            curr_router_y_pos,
                            0,
                            DUMMY_LATENCY);
    }

    // now go through and add the links to the NoC

    // allocate the size for outgoing link vector for each router
    test_noc.make_room_for_noc_router_link_list();

    for (int source_router_id = 0; source_router_id < NUM_OF_ROUTERS; source_router_id++) {
        source = (NocRouterId)source_router_id;

        for (int sink_router_id = 0; sink_router_id < NOC_CONNECTIVITY; sink_router_id++) {
            sink = (NocRouterId)sink_router_id;

            // makes sure we do not create a link for a router who acts as a sink and source
            if (source_router_id != sink_router_id) {
                // add the link to the NoC
                test_noc.add_link(source, sink, DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
        }
    }

    SECTION("Test case where a legal link is removed from the NoC.") {
        // choose a source router and a sink router of a link that we want to remove
        NocRouterId link_to_remove_src_router = NocRouterId(src_router(rand_num_gen));
        NocRouterId link_to_remove_sink_router;

        do {
            link_to_remove_sink_router = NocRouterId(sink_router(rand_num_gen));
        } while (link_to_remove_src_router == link_to_remove_sink_router);

        // run the test function, which is to delete a link with the source and sink router found above
        REQUIRE_NOTHROW(test_noc.remove_link(link_to_remove_src_router, link_to_remove_sink_router) == true);

        /* now verify whether the link was removed correctly */

        // variable to keep track of whether the link was deleted from the vector outgoing links of its source router
        bool link_removed_from_outgoing_vector = true;

        auto& outgoing_links = test_noc.get_noc_router_outgoing_links(link_to_remove_src_router);
        // go through all the outgoing links  of the source router in the link we removed and check that the link does not exist there as well.
        for (auto outgoing_link : outgoing_links) {
            // get the current outgoing link
            const NocLink& curr_outgoing_link = test_noc.get_single_noc_link(outgoing_link);

            if ((curr_outgoing_link.get_source_router() == link_to_remove_src_router) && (curr_outgoing_link.get_sink_router() == link_to_remove_sink_router)) {
                link_removed_from_outgoing_vector = false;
                break;
            }
        }

        // verify that the link was set to be in invalid inside the vector of all links in the NoC
        const auto& links_in_noc = test_noc.get_noc_links();
        // go through the links and make sure that none of them have the source and sink router of the link
        // that we removed. THe removed link should have the source and sink routers set to invalid values.
        for (auto single_link : links_in_noc) {
            // check whether the source and sink router of the current link matches the routers in the link to remove
            if ((single_link.get_source_router() == link_to_remove_src_router) && (single_link.get_sink_router() == link_to_remove_sink_router)) {
                // this indicates that the link was not set to an invalid state and not removed properly
                link_removed_from_outgoing_vector = false;
                break;
            }
        }

        // verify the status of whether the link was removed as an outgoing link from the source router
        REQUIRE(link_removed_from_outgoing_vector == true);
    }
    SECTION("Test the case where the link to remove does not exist in the NoC.") {
        // choose a source router and a sink router of a link that we want to remove. These routers are chosen so that the link does not exist in the NoC.
        NocRouterId link_to_remove_src_router = NocRouterId(NUM_OF_ROUTERS + 1);
        NocRouterId link_to_remove_sink_router = NocRouterId(NUM_OF_ROUTERS + 2);

        // run the test function, which is to delete a link with the source and sink router found above. We expect the status of the function to indicate that the deletion failed
        REQUIRE(test_noc.remove_link(link_to_remove_src_router, link_to_remove_sink_router) == false);
    }
}
TEST_CASE("test_generate_router_key_from_grid_location", "[vpr_noc]") {
    /**
     * This unit test verifies whether the function which determines the 
     * physical router located an a given grid location works correctly.
     * 
     * We start by creating a NoC model and adding routers to it. We
     * store the router location to its id inside a golden set. Then
     * we execute the test function and verify that the routers it 
     * returns for a given grid location matches our golden set.
     */

    // will store all routers in a given design
    // The index represents the x and y location of the router found at the given index
    // We will use this test to verify if the test function is returning the correct router for a given grid location
    std::vector<NocRouterId> golden_set;

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    // testing data structure
    NocStorage test_noc;

    // store the reference to device grid with
    // this will be set to the total number of routers (and should be set before adding routers)
    test_noc.set_device_grid_spec((int)NUM_OF_ROUTERS, 0);

    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number;
        router_grid_position_y = router_number;

        // add the current router_id to the golden vector (the id is determined similar to how it is done in add_router())
        // this vector is built so that the index represents the grid location of the current router
        golden_set.emplace_back((NocRouterId)router_number);

        // add the router to the noc
        test_noc.add_router(curr_router_id,
                            router_grid_position_x,
                            router_grid_position_y,
                            0,
                            DUMMY_LATENCY);
    }

    // now verify the test function by identifying all the routers using their grid locations
    // the grid locations go from 0 to the total number of routers in the NoC
    for (int grid_location = 0; grid_location < NUM_OF_ROUTERS; grid_location++) {
        // contains the grid location of a router block seen during placement
        // we don't care about the subtile so give it an arbitrary value
        t_pl_loc placement_router_grid_location = t_pl_loc(grid_location,
                                                           grid_location,
                                                           -1,
                                                           0);

        NocRouterId found_router_at_grid_location = test_noc.get_router_at_grid_location(placement_router_grid_location);

        // verify that the found router matches the expected result
        REQUIRE(golden_set[grid_location] == found_router_at_grid_location);
    }
}

} // namespace