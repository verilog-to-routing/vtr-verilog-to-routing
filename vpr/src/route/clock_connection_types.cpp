#include "clock_connection_types.h"

#include "globals.h"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_error.h"

/*
 * RoutingToClockConnection (getters)
 */

ClockConnectionType RoutingToClockConnection::get_connection_type() const {
    return ClockConnectionType::ROUTING_TO_CLOCK;
}

/*
 * RoutingToClockConnection (setters)
 */

void RoutingToClockConnection::set_clock_name_to_connect_to(std::string clock_name) {
    clock_to_connect_to = clock_name;
}

void RoutingToClockConnection::set_clock_switch_name(std::string clock_switch_name) {
    switch_name = clock_switch_name;
}

void RoutingToClockConnection::set_switch_location(int x, int y) {
    switch_location.x = x;
    switch_location.y = y;
}

void RoutingToClockConnection::set_switch(int switch_index) {
    switch_idx = switch_index;
}

void RoutingToClockConnection::set_fc_val(int fc_val) {
    fc = fc_val;
}

/*
 * RoutingToClockConnection (member functions)
 */

void RoutingToClockConnection::create_switches(const ClockRRGraph& clock_graph) {
    
}




/*
 * ClockToClockConneciton (getters)
 */

ClockConnectionType ClockToClockConneciton::get_connection_type() const {
    return ClockConnectionType::CLOCK_TO_CLOCK;
}

/*
 * ClockToClockConneciton (setters)
 */

void ClockToClockConneciton::set_from_clock_name(std::string clock_name) {
    from_clock = clock_name;
}

void ClockToClockConneciton::set_from_clock_switch_name(std::string switch_name) {
    from_switch = switch_name;
}

void ClockToClockConneciton::set_to_clock_name(std::string clock_name) {
    to_clock = clock_name;
}

void ClockToClockConneciton::set_to_clock_switch_name(std::string switch_name) {
    to_switch = switch_name;
}

void ClockToClockConneciton::set_switch(int switch_index) {
    switch_idx = switch_index;
}

void ClockToClockConneciton::set_fc_val(int fc_val) {
    fc = fc_val;
}

/*
 * ClockToClockConneciton (member functions)
 */

void ClockToClockConneciton::create_switches(const ClockRRGraph& clock_graph) {

}




/*
 * ClockToPinsConnection (getters)
 */

ClockConnectionType ClockToPinsConnection::get_connection_type() const {
    return ClockConnectionType::CLOCK_TO_PINS;
}

/*
 * ClockToPinsConnection (setters)
 */

void ClockToPinsConnection::set_clock_name_to_connect_from(std::string clock_name) {
    clock_to_connect_from = clock_name;
}

void ClockToPinsConnection::set_clock_switch_name(std::string connection_switch_name) {
    switch_name = connection_switch_name;
}

void ClockToPinsConnection::set_switch(int switch_index) {
    switch_idx = switch_index;
}

void ClockToPinsConnection::set_fc_val(int fc_val) {
    fc = fc_val;
}

/*
 * ClockToPinsConnection (member functions)
 */

void ClockToPinsConnection::create_switches(const ClockRRGraph& clock_graph) {

}




/*
 * RoutingToPins (getters)
 */

ClockConnectionType  RoutingToPins::get_connection_type() const {
    return ClockConnectionType::ROUTING_TO_PINS;
}

/*
 * RoutingToPins (setters)
 */

void RoutingToPins::set_fc_val(int fc_val) {
    fc = fc_val;
}

/*
 * RoutingToPins (member functions)
 */

void RoutingToPins::create_switches(const ClockRRGraph& clock_graph) {
    (void)clock_graph; // parameter not needed in this case
}

