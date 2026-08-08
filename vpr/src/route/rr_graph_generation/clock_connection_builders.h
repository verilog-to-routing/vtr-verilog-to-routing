#pragma once

#include <string>

#include "rr_graph_clock.h"
#include "vpr_types.h"

class ClockRRGraphBuilder;

/// @brief Which two kinds of RR node a ClockConnection joins.
enum class ClockConnectionType {
    ROUTING_TO_CLOCK,
    CLOCK_TO_CLOCK,
    CLOCK_TO_PINS,
    ROUTING_TO_PINS
};

/// @brief Base class for a single <clock_routing> <tap>: an edge (or set of edges,
/// fc-limited) between a clock network's switch point and some other part of the RR
/// graph. One concrete subclass exists per ClockConnectionType.
class ClockConnection {
  public:
    virtual ~ClockConnection() {}

    virtual void create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) = 0;
    virtual size_t estimate_additional_nodes() = 0;
};

/// @brief Connects the inter-block routing (CHANX/CHANY wires) to a clock network's
/// drive point at the specified coordinates.
class RoutingToClockConnection : public ClockConnection {
  private:
    std::string clock_to_connect_to;
    std::string switch_point_name;
    t_physical_tile_loc switch_location;
    // Representative location for this clock network's shared virtual sink node (see
    // create_switches). Only used the first time that node is created; ignored on
    // later reuse. Set to the centroid of all of this clock network's drive points,
    // not this connection's own switch_location.
    t_physical_tile_loc virtual_sink_location;
    int arch_switch_idx = UNDEFINED;
    float fc = 0.;

    int seed = 101;

  public:
    // Setters
    void set_clock_name_to_connect_to(std::string clock_name);
    void set_clock_switch_point_name(std::string clock_switch_point_name);
    void set_switch_location(int x, int y, int layer = 0);
    void set_virtual_sink_location(int x, int y, int layer = 0);
    void set_switch(int arch_switch_index);
    void set_fc_val(float fc_val);

    // Member functions
    void create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) override;
    size_t estimate_additional_nodes() override;
};

/// @brief Connects one clock network's switch point (the "from" side) to another clock
/// network's switch point (the "to" side), e.g. a rib driving a spine.
class ClockToClockConnection : public ClockConnection {
  private:
    std::string from_clock;
    std::string from_switch;
    std::string to_clock;
    std::string to_switch;
    int arch_switch_idx = UNDEFINED;
    float fc = 0.;

  public:
    // Setters
    void set_from_clock_name(std::string clock_name);
    void set_from_clock_switch_point_name(std::string switch_point_name);
    void set_to_clock_name(std::string clock_name);
    void set_to_clock_switch_point_name(std::string switch_point_name);
    void set_switch(int arch_switch_index);
    void set_fc_val(float fc_val);

    // Member functions
    void create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) override;
    size_t estimate_additional_nodes() override;
};

/// @brief Connects a specific tile port/pin range at a single grid location to a clock
/// network drive point, forming a mux from all of the specified pins. Used for the
/// "TILE.<tile_name>[hi:lo].<port_name>[hi:lo]" tap syntax, as an alternative to driving
/// a clock network from general-purpose routing (see RoutingToClockConnection).
class TileToClockConnection : public ClockConnection {
  private:
    std::string clock_to_connect_to;
    std::string switch_point_name;
    t_physical_tile_loc location;
    // Representative location for this clock network's shared virtual sink node (see
    // get_or_create_virtual_clock_network_root). Only used the first time that node is
    // created; ignored on later reuse. Set to the centroid of all of this clock
    // network's drive points, not this connection's own location.
    t_physical_tile_loc virtual_sink_location;
    std::string tile_name;
    std::string port_name;
    // {-1, -1} means "not specified in the architecture file", i.e. use the full
    // range (all sub tile instances / all pins of the port).
    std::pair<int, int> subtile_range = {-1, -1};
    std::pair<int, int> pin_range = {-1, -1};
    int arch_switch_idx = UNDEFINED;
    float fc = 0.;

    int seed = 101;

  public:
    // Setters
    void set_clock_name_to_connect_to(std::string clock_name);
    void set_clock_switch_point_name(std::string clock_switch_point_name);
    void set_location(int x, int y, int layer = 0);
    void set_virtual_sink_location(int x, int y, int layer = 0);
    void set_tile_name(std::string name);
    void set_subtile_range(int low, int high);
    void set_port_name(std::string name);
    void set_pin_range(int low, int high);
    void set_switch(int arch_switch_index);
    void set_fc_val(float fc_val);

    // Member functions
    void create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) override;
    size_t estimate_additional_nodes() override;
};

/// @brief Connects a clock network's switch point to block clock pins.
/// @note Currently only supports connecting to dedicated clock pins, not to arbitrary
/// block pins.
class ClockToPinsConnection : public ClockConnection {
  private:
    std::string clock_to_connect_from;
    std::string switch_point_name;
    int arch_switch_idx = UNDEFINED;
    float fc = 0.;

  public:
    // Setters
    void set_clock_name_to_connect_from(std::string clock_name);
    void set_clock_switch_point_name(std::string connection_switch_point_name);
    void set_switch(int arch_switch_index);
    void set_fc_val(float fc_val);

    // Member functions
    void create_switches(const ClockRRGraphBuilder& clock_graph, t_rr_edge_info_set* rr_edges_to_create) override;
    size_t estimate_additional_nodes() override;
};
